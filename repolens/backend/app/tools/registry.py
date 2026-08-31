"""Tool schemas (Anthropic tool-use format) and a single dispatch function.

Adding a new tool means: write the function, add its schema here, add one
line to `execute_tool`'s dispatch table.
"""

from __future__ import annotations

from pathlib import Path
from typing import Any

from app.paths import PathSecurityError
from app.tools.code_tools import search_code
from app.tools.file_tools import list_directory, read_file, search_files
from app.tools.git_tools import get_git_history

TOOL_SCHEMAS: list[dict[str, Any]] = [
    {
        "name": "search_files",
        "description": (
            "Find files in the repository by filename or path pattern. Use this to "
            "locate files when you know part of a name (e.g. 'auth', 'routes/*.js'). "
            "Supports glob patterns (*, ?) or a plain substring match."
        ),
        "input_schema": {
            "type": "object",
            "properties": {
                "pattern": {"type": "string", "description": "Substring or glob to match against file paths, e.g. 'auth' or 'src/routes/*.ts'"},
                "max_results": {"type": "integer", "description": "Maximum number of file paths to return (default 50)"},
            },
            "required": ["pattern"],
        },
    },
    {
        "name": "search_code",
        "description": (
            "Search file CONTENTS across the repository for a keyword, function name, "
            "or regex (e.g. 'authenticate', 'jwt.sign', 'class UserRepository'). Returns "
            "matching file paths, line numbers, and the matching line. This is the "
            "primary way to find where a concept is implemented."
        ),
        "input_schema": {
            "type": "object",
            "properties": {
                "query": {"type": "string", "description": "Text or regex to search for"},
                "regex": {"type": "boolean", "description": "Treat query as a regular expression (default false)"},
                "file_glob": {"type": "string", "description": "Optional glob to restrict search, e.g. '*.py' or 'src/**/*.ts'"},
                "max_results": {"type": "integer", "description": "Maximum number of matches to return (default 30)"},
            },
            "required": ["query"],
        },
    },
    {
        "name": "read_file",
        "description": (
            "Read the contents of a specific file (optionally a line range) with line "
            "numbers included. Use this after search_files/search_code point you at a "
            "file, to read the actual implementation before answering."
        ),
        "input_schema": {
            "type": "object",
            "properties": {
                "path": {"type": "string", "description": "File path relative to the repository root"},
                "start_line": {"type": "integer", "description": "Optional 1-indexed start line"},
                "end_line": {"type": "integer", "description": "Optional 1-indexed end line"},
            },
            "required": ["path"],
        },
    },
    {
        "name": "list_directory",
        "description": "List the immediate files and subdirectories of a directory. Use this to explore the repository's structure.",
        "input_schema": {
            "type": "object",
            "properties": {
                "path": {"type": "string", "description": "Directory path relative to the repository root, default '.'"},
            },
            "required": [],
        },
    },
    {
        "name": "get_git_history",
        "description": (
            "Get recent git commit history for the whole repo or a specific file. "
            "Useful for understanding when/why a feature was added or last changed."
        ),
        "input_schema": {
            "type": "object",
            "properties": {
                "path": {"type": "string", "description": "Optional file path to scope history to"},
                "max_commits": {"type": "integer", "description": "Maximum number of commits to return (default 20)"},
            },
            "required": [],
        },
    },
]


def execute_tool(root: Path, name: str, tool_input: dict[str, Any]) -> dict[str, Any]:
    """Dispatch a tool call by name. Never raises: errors come back as
    {"error": "..."} so the agent loop can feed them back to the model.
    """
    try:
        if name == "search_files":
            return search_files(
                root,
                pattern=tool_input.get("pattern", ""),
                max_results=int(tool_input.get("max_results", 50)),
            )
        if name == "search_code":
            return search_code(
                root,
                query=tool_input.get("query", ""),
                regex=bool(tool_input.get("regex", False)),
                file_glob=tool_input.get("file_glob"),
                max_results=int(tool_input.get("max_results", 30)),
            )
        if name == "read_file":
            return read_file(
                root,
                path=tool_input.get("path", ""),
                start_line=tool_input.get("start_line"),
                end_line=tool_input.get("end_line"),
            )
        if name == "list_directory":
            return list_directory(root, path=tool_input.get("path", "."))
        if name == "get_git_history":
            return get_git_history(
                root,
                path=tool_input.get("path"),
                max_commits=int(tool_input.get("max_commits", 20)),
            )
        return {"error": f"Unknown tool '{name}'"}
    except PathSecurityError as exc:
        return {"error": str(exc)}
    except Exception as exc:  # last line of defense: never crash the agent loop
        return {"error": f"Tool '{name}' failed: {exc}"}
