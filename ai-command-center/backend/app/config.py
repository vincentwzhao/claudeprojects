from pydantic_settings import BaseSettings, SettingsConfigDict


class Settings(BaseSettings):
    model_config = SettingsConfigDict(env_file=".env", extra="ignore")

    anthropic_api_key: str = ""
    tavily_api_key: str = ""
    claude_model: str = "claude-sonnet-4-5-20250929"
    database_url: str = "sqlite:///./ai_command_center.db"
    upload_dir: str = "./uploads"
    cors_origins: str = "http://localhost:5173"

    @property
    def cors_origin_list(self) -> list[str]:
        return [o.strip() for o in self.cors_origins.split(",") if o.strip()]


settings = Settings()
