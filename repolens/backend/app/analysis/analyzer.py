"""Heuristic, deterministic repository analysis for the /project overview.

This never calls an LLM: it's plain static analysis (extension counts,
dependency manifests, regex scans for routes/auth/db keywords). The goal is
"good enough on common projects", not perfect understanding of every repo.
The output feeds the /project prompt as grounded facts the LLM narrates,
and is also returned as raw JSON for the UI's Architecture/Dependencies tabs.
"""

from __future__ import annotations

import json
import os
import re
from dataclasses import asdict, dataclass, field
from pathlib import Path

from app.config import settings
from app.paths import EXCLUDED_DIRS, iter_source_files, to_relative

LANGUAGE_EXTENSIONS = {
    ".py": "Python", ".js": "JavaScript", ".jsx": "JavaScript (JSX)",
    ".mjs": "JavaScript", ".cjs": "JavaScript",
    ".ts": "TypeScript", ".tsx": "TypeScript (JSX)",
    ".go": "Go", ".rb": "Ruby", ".java": "Java", ".kt": "Kotlin",
    ".rs": "Rust", ".php": "PHP", ".c": "C", ".h": "C",
    ".cpp": "C++", ".hpp": "C++", ".cs": "C#", ".swift": "Swift",
    ".scala": "Scala", ".ex": "Elixir", ".exs": "Elixir",
    ".html": "HTML", ".css": "CSS", ".scss": "SCSS", ".vue": "Vue",
    ".sql": "SQL", ".sh": "Shell",
}

CODE_EXTENSIONS = {".py", ".js", ".jsx", ".mjs", ".cjs", ".ts", ".tsx", ".go",
                    ".rb", ".java", ".kt", ".rs", ".php", ".cs", ".vue"}

FRAMEWORK_DEP_MARKERS = {
    "express": "Express (Node.js)", "fastify": "Fastify (Node.js)",
    "koa": "Koa (Node.js)", "@nestjs/core": "NestJS", "next": "Next.js",
    "react": "React", "vue": "Vue.js", "nuxt": "Nuxt.js",
    "@angular/core": "Angular", "svelte": "Svelte",
    "fastapi": "FastAPI (Python)", "flask": "Flask (Python)",
    "django": "Django (Python)", "tornado": "Tornado (Python)",
    "aiohttp": "aiohttp (Python)", "rails": "Ruby on Rails",
    "gin-gonic/gin": "Gin (Go)", "labstack/echo": "Echo (Go)",
    "gorilla/mux": "Gorilla Mux (Go)", "spring-boot": "Spring Boot (Java)",
    "laravel/framework": "Laravel (PHP)",
}

DB_MARKERS = {
    "mongoose": "MongoDB (Mongoose)", "sequelize": "SQL (Sequelize ORM)",
    "prisma": "SQL/NoSQL (Prisma ORM)", "typeorm": "SQL (TypeORM)",
    "knex": "SQL (Knex query builder)", "pg": "PostgreSQL",
    "mysql2": "MySQL", "mysql": "MySQL", "sqlite3": "SQLite",
    "redis": "Redis", "mongodb": "MongoDB", "sqlalchemy": "SQL (SQLAlchemy)",
    "psycopg2": "PostgreSQL (Python)", "pymongo": "MongoDB (Python)",
    "gorm.io/gorm": "SQL (GORM, Go)", "django.db": "SQL (Django ORM)",
}

AUTH_MARKERS = {
    "jsonwebtoken": "JWT (jsonwebtoken)", "jwt": "JWT",
    "passport": "Passport.js", "bcrypt": "bcrypt password hashing",
    "bcryptjs": "bcrypt password hashing", "next-auth": "NextAuth.js",
    "express-session": "Session-based auth", "flask_login": "Flask-Login",
    "flask-jwt-extended": "Flask JWT", "django.contrib.auth": "Django auth",
    "devise": "Devise (Rails auth)", "spring-security": "Spring Security",
    "firebase-auth": "Firebase Auth", "auth0": "Auth0", "oauth": "OAuth",
    "pyjwt": "JWT (PyJWT)",
}

EXTERNAL_SERVICE_MARKERS = {
    "stripe": "Stripe (payments)", "twilio": "Twilio (SMS)",
    "sendgrid": "SendGrid (email)", "aws-sdk": "AWS", "@aws-sdk": "AWS",
    "boto3": "AWS (Python)", "google-cloud": "Google Cloud",
    "openai": "OpenAI API", "anthropic": "Anthropic API",
    "firebase": "Firebase", "cloudinary": "Cloudinary",
    "algolia": "Algolia", "mailgun": "Mailgun", "plaid": "Plaid",
}

ENTRY_POINT_CANDIDATES = [
    "main.py", "app.py", "manage.py", "wsgi.py", "asgi.py",
    "index.js", "index.ts", "server.js", "server.ts", "app.js", "app.ts",
    "main.go", "main.rs", "Main.java", "Program.cs",
    "src/main.tsx", "src/main.ts", "src/index.tsx", "src/index.ts", "src/index.js",
]

IMPORTANT_DIR_HINTS = {
    "routes": "HTTP route definitions", "controllers": "Request handlers / controllers",
    "services": "Business logic layer", "repositories": "Data access layer",
    "models": "Data models / ORM schemas", "middleware": "Request middleware (auth, logging, etc.)",
    "config": "Configuration", "tests": "Automated tests", "test": "Automated tests",
    "__tests__": "Automated tests", "utils": "Shared utilities", "lib": "Shared libraries",
    "components": "UI components", "pages": "Page-level UI / routes",
    "api": "API layer", "db": "Database layer", "database": "Database layer",
    "migrations": "Database migrations", "schemas": "Data schemas / validation",
    "views": "View templates / handlers", "public": "Static assets",
    "static": "Static assets", "scripts": "Utility/build scripts", "docs": "Documentation",
    "handlers": "Request handlers", "hooks": "React hooks", "store": "State management",
}

ROUTE_PATTERNS = [
    (re.compile(r"\b(?:app|router)\.(get|post|put|patch|delete)\s*\(\s*[\"'`]([^\"'`]+)[\"'`]", re.I), "Express-style"),
    (re.compile(r"@(?:app|router)\.(get|post|put|patch|delete)\s*\(\s*[\"']([^\"']+)[\"']", re.I), "FastAPI"),
    (re.compile(r"@(?:app|bp)\.route\s*\(\s*[\"']([^\"']+)[\"']"), "Flask"),
    (re.compile(r"\bpath\s*\(\s*[\"']([^\"']*)[\"']"), "Django-urls"),
    (re.compile(r"@(GetMapping|PostMapping|PutMapping|PatchMapping|DeleteMapping)\s*\(\s*[\"']([^\"']+)[\"']"), "Spring"),
    (re.compile(r"\.(GET|POST|PUT|PATCH|DELETE)\s*\(\s*\"([^\"]+)\""), "Go-router"),
]

SECRET_PATTERN = re.compile(
    r"\b(API_KEY|SECRET|PASSWORD|PRIVATE_KEY|ACCESS_TOKEN)\s*[:=]\s*[\"']([A-Za-z0-9_\-/+]{8,})[\"']"
)

MAX_CONTENT_SCAN_FILES = 4000
MAX_FILE_SCAN_BYTES = 200_000


@dataclass
class RepositoryAnalysis:
    languages: dict = field(default_factory=dict)
    frameworks: list = field(default_factory=list)
    entry_points: list = field(default_factory=list)
    important_directories: list = field(default_factory=list)
    major_modules: list = field(default_factory=list)
    api_routes: list = field(default_factory=list)
    api_route_count: int = 0
    database_layer: list = field(default_factory=list)
    authentication: list = field(default_factory=list)
    external_services: list = field(default_factory=list)
    configuration: list = field(default_factory=list)
    tests: dict = field(default_factory=dict)
    build_system: list = field(default_factory=list)
    dependencies: dict = field(default_factory=dict)
    potential_issues: list = field(default_factory=list)
    file_count: int = 0

    def to_dict(self) -> dict:
        return asdict(self)


def _parse_package_json(path: Path) -> tuple[list[str], dict]:
    try:
        data = json.loads(path.read_text(encoding="utf-8", errors="ignore"))
    except (OSError, json.JSONDecodeError):
        return [], {}
    deps = {**data.get("dependencies", {}), **data.get("devDependencies", {})}
    return list(deps.keys()), deps


def _parse_requirements_txt(path: Path) -> list[str]:
    names = []
    try:
        for line in path.read_text(encoding="utf-8", errors="ignore").splitlines():
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            name = re.split(r"[=<>!~\[]", line, maxsplit=1)[0].strip()
            if name:
                names.append(name.lower())
    except OSError:
        pass
    return names


def _parse_pyproject_toml(path: Path) -> list[str]:
    names = []
    try:
        text = path.read_text(encoding="utf-8", errors="ignore")
    except OSError:
        return names
    for match in re.finditer(r'^\s*"?([A-Za-z0-9_.\-]+)"?\s*=\s*[">^~=\d.]', text, re.M):
        names.append(match.group(1).lower())
    for match in re.finditer(r'^\s*"([A-Za-z0-9_.\-]+)(?:[><=!~^].*)?"\s*,?\s*$', text, re.M):
        names.append(match.group(1).lower())
    return names


def analyze_repository(root: Path) -> RepositoryAnalysis:
    analysis = RepositoryAnalysis()
    root = root.resolve()

    language_counts: dict[str, int] = {}
    top_level_dirs: dict[str, int] = {}
    file_count = 0

    for full_path in iter_source_files(root, limit=20_000):
        file_count += 1
        ext = full_path.suffix.lower()
        lang = LANGUAGE_EXTENSIONS.get(ext)
        if lang:
            language_counts[lang] = language_counts.get(lang, 0) + 1

        rel = to_relative(root, full_path)
        top = rel.split("/", 1)[0]
        if top != rel:
            top_level_dirs[top] = top_level_dirs.get(top, 0) + 1

    analysis.file_count = file_count
    analysis.languages = dict(sorted(language_counts.items(), key=lambda kv: -kv[1]))

    # --- important directories & major modules -----------------------------
    analysis.major_modules = [
        {"name": name, "file_count": count}
        for name, count in sorted(top_level_dirs.items(), key=lambda kv: -kv[1])[:12]
    ]

    # Walk a few levels deep (common layouts nest routes/controllers/etc.
    # under src/, src/app/, etc.) and record the shallowest match for each
    # recognized directory name, so e.g. "src/app/routes" is found even
    # though "routes" isn't a top-level directory.
    MAX_DIR_SCAN_DEPTH = 4
    found_hints: dict[str, str] = {}
    root_depth = len(root.parts)
    for dirpath, dirnames, _ in os.walk(root):
        dirnames[:] = sorted(d for d in dirnames if d not in EXCLUDED_DIRS)
        depth = len(Path(dirpath).parts) - root_depth
        if depth >= MAX_DIR_SCAN_DEPTH:
            dirnames[:] = []
            continue
        for dname in dirnames:
            if dname in IMPORTANT_DIR_HINTS and dname not in found_hints:
                full = Path(dirpath) / dname
                found_hints[dname] = to_relative(root, full)

    for dname, rel_path in found_hints.items():
        full = root / rel_path
        analysis.important_directories.append({
            "path": rel_path,
            "description": IMPORTANT_DIR_HINTS[dname],
            "file_count": sum(1 for _ in full.rglob("*") if _.is_file()),
        })
    analysis.important_directories.sort(key=lambda d: d["path"])

    # --- entry points --------------------------------------------------------
    for candidate in ENTRY_POINT_CANDIDATES:
        if (root / candidate).is_file():
            analysis.entry_points.append({"path": candidate, "reason": "conventional entry file"})

    pkg_json = root / "package.json"
    if pkg_json.is_file():
        try:
            data = json.loads(pkg_json.read_text(encoding="utf-8", errors="ignore"))
            main = data.get("main")
            if main and (root / main).is_file():
                analysis.entry_points.append({"path": main, "reason": "package.json 'main'"})
            for script_name, script_cmd in (data.get("scripts") or {}).items():
                if script_name in ("start", "dev", "serve"):
                    analysis.build_system.append({"tool": "npm", "script": f"{script_name}: {script_cmd}"})
        except (OSError, json.JSONDecodeError):
            pass

    # --- dependencies & frameworks -------------------------------------------
    all_dep_names: set[str] = set()
    if pkg_json.is_file():
        names, versioned = _parse_package_json(pkg_json)
        all_dep_names |= {n.lower() for n in names}
        analysis.dependencies["npm"] = [{"name": n, "version": v} for n, v in list(versioned.items())[:60]]
        analysis.build_system.append({"tool": "npm/yarn/pnpm", "manifest": "package.json"})

    req_txt = root / "requirements.txt"
    if req_txt.is_file():
        names = _parse_requirements_txt(req_txt)
        all_dep_names |= set(names)
        analysis.dependencies["pip"] = [{"name": n} for n in names[:60]]
        analysis.build_system.append({"tool": "pip", "manifest": "requirements.txt"})

    pyproject = root / "pyproject.toml"
    if pyproject.is_file():
        names = _parse_pyproject_toml(pyproject)
        all_dep_names |= set(names)
        analysis.dependencies.setdefault("pip", [])
        analysis.build_system.append({"tool": "poetry/pip", "manifest": "pyproject.toml"})

    for manifest, tool in [("go.mod", "go"), ("Cargo.toml", "cargo"), ("Gemfile", "bundler"),
                            ("composer.json", "composer"), ("pom.xml", "maven"), ("build.gradle", "gradle")]:
        mpath = root / manifest
        if mpath.is_file():
            analysis.build_system.append({"tool": tool, "manifest": manifest})
            try:
                text = mpath.read_text(encoding="utf-8", errors="ignore").lower()
                all_dep_names |= set(re.findall(r"[a-z0-9_./\-]{3,40}", text))
            except OSError:
                pass

    if (root / "Dockerfile").is_file():
        analysis.build_system.append({"tool": "docker", "manifest": "Dockerfile"})
    if (root / "docker-compose.yml").is_file() or (root / "docker-compose.yaml").is_file():
        analysis.build_system.append({"tool": "docker-compose", "manifest": "docker-compose.yml"})
    if (root / "Makefile").is_file():
        analysis.build_system.append({"tool": "make", "manifest": "Makefile"})
    if (root / ".github" / "workflows").is_dir():
        analysis.build_system.append({"tool": "GitHub Actions", "manifest": ".github/workflows"})

    seen_db_labels: set[str] = set()
    seen_auth_labels: set[str] = set()
    seen_external_labels: set[str] = set()

    for marker, label in FRAMEWORK_DEP_MARKERS.items():
        if any(marker.lower() in dep for dep in all_dep_names) and label not in analysis.frameworks:
            analysis.frameworks.append(label)
    for marker, label in DB_MARKERS.items():
        if label in seen_db_labels:
            continue
        if any(marker.lower() == dep or marker.lower() in dep for dep in all_dep_names):
            analysis.database_layer.append({"type": label, "evidence": f"dependency: {marker}"})
            seen_db_labels.add(label)
    for marker, label in AUTH_MARKERS.items():
        if label in seen_auth_labels:
            continue
        if any(marker.lower() in dep for dep in all_dep_names):
            analysis.authentication.append({"type": label, "evidence": f"dependency: {marker}"})
            seen_auth_labels.add(label)
    for marker, label in EXTERNAL_SERVICE_MARKERS.items():
        if label in seen_external_labels:
            continue
        if any(marker.lower() in dep for dep in all_dep_names):
            analysis.external_services.append({"type": label, "evidence": f"dependency: {marker}"})
            seen_external_labels.add(label)

    # --- configuration files --------------------------------------------------
    for cfg in [".env", ".env.example", ".env.sample", "config.py", "settings.py",
                "config.js", "config.ts", "docker-compose.yml", ".env.production"]:
        if (root / cfg).is_file():
            analysis.configuration.append(cfg)
    for cfg_dir in ["config", "settings"]:
        if (root / cfg_dir).is_dir():
            analysis.configuration.append(f"{cfg_dir}/")

    # --- tests -----------------------------------------------------------------
    test_dirs = [d for d in ("tests", "test", "__tests__", "spec") if (root / d).is_dir()]
    test_framework = None
    for marker, fw in [("jest", "Jest"), ("mocha", "Mocha"), ("vitest", "Vitest"),
                        ("pytest", "pytest"), ("rspec", "RSpec")]:
        if marker in all_dep_names:
            test_framework = fw
            break
    if not test_framework and ((root / "pytest.ini").is_file() or (root / "conftest.py").is_file()):
        test_framework = "pytest"
    analysis.tests = {"directories": test_dirs, "framework": test_framework}

    # --- content scan: routes + secrets (bounded) -------------------------------
    scanned = 0
    secret_hits = []
    route_seen = set()
    for full_path in iter_source_files(root, limit=settings.max_repo_index_files):
        if full_path.suffix.lower() not in CODE_EXTENSIONS:
            continue
        if scanned >= MAX_CONTENT_SCAN_FILES:
            break
        try:
            if full_path.stat().st_size > MAX_FILE_SCAN_BYTES:
                continue
            text = full_path.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue
        scanned += 1
        rel = to_relative(root, full_path)

        for pattern, framework in ROUTE_PATTERNS:
            for m in pattern.finditer(text):
                groups = m.groups()
                if len(groups) == 2:
                    method, route_path = groups
                    method = method.upper() if method.upper() in {"GET", "POST", "PUT", "PATCH", "DELETE"} else "ROUTE"
                else:
                    method, route_path = "ROUTE", groups[0]
                key = (method, route_path, rel)
                if key in route_seen or not route_path:
                    continue
                route_seen.add(key)
                line_no = text[: m.start()].count("\n") + 1
                analysis.api_routes.append({
                    "method": method, "path": route_path, "file": rel, "line": line_no, "framework": framework,
                })

        for m in SECRET_PATTERN.finditer(text):
            if ".example" in rel or ".sample" in rel:
                continue
            secret_hits.append(f"{rel}:{text[: m.start()].count(chr(10)) + 1}")

    analysis.api_route_count = len(analysis.api_routes)
    analysis.api_routes = analysis.api_routes[:60]

    # --- potential issues heuristics --------------------------------------------
    if not test_dirs and not test_framework:
        analysis.potential_issues.append("No dedicated test directory or test framework detected — test coverage is unclear.")
    if not (root / "README.md").is_file() and not (root / "README").is_file():
        analysis.potential_issues.append("No README found at the repository root.")
    if not (root / ".gitignore").is_file():
        analysis.potential_issues.append("No .gitignore found.")
    workflows_dir = root / ".github" / "workflows"
    has_ci = workflows_dir.is_dir() and any(workflows_dir.glob("*"))
    if not has_ci:
        analysis.potential_issues.append("No CI configuration detected (e.g. GitHub Actions).")
    if secret_hits:
        analysis.potential_issues.append(
            f"Possible hardcoded secrets found (review before shipping): {', '.join(secret_hits[:5])}"
        )
    if not analysis.authentication and any(r["path"] for r in analysis.api_routes):
        analysis.potential_issues.append("API routes were found but no authentication library was detected — verify routes are protected appropriately.")

    return analysis
