from pydantic_settings import BaseSettings, SettingsConfigDict


class Settings(BaseSettings):
    """Application configuration, loaded from environment / .env file.

    Secrets (the API key) are never hardcoded and never returned by any API
    endpoint.
    """

    model_config = SettingsConfigDict(env_file=".env", extra="ignore")

    anthropic_api_key: str = ""
    claude_model: str = "claude-sonnet-4-5-20250929"
    workspace_dir: str = "./workspace"
    cors_origins: str = "http://localhost:5173"
    max_agent_iterations: int = 8

    # Safety limits so a single request can't blow up memory/tokens.
    max_file_read_bytes: int = 60_000
    max_search_results: int = 60
    max_repo_index_files: int = 20_000


settings = Settings()
