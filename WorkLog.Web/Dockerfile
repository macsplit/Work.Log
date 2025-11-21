# Work Log Web - Multi-stage Docker build

# Build stage
FROM mcr.microsoft.com/dotnet/sdk:8.0 AS build
WORKDIR /src

# Copy project files
COPY Domain/WorkLog.Domain.csproj Domain/
COPY Website/WorkLog.Website.csproj Website/

# Restore dependencies
RUN dotnet restore Website/WorkLog.Website.csproj

# Copy source code
COPY . .

# Build the application
WORKDIR /src/Website
RUN dotnet build -c Release -o /app/build

# Publish stage
FROM build AS publish
RUN dotnet publish -c Release -o /app/publish /p:UseAppHost=false

# Runtime stage
FROM mcr.microsoft.com/dotnet/aspnet:8.0 AS final
WORKDIR /app

# Create data directory for SQLite database
RUN mkdir -p /data && chown -R app:app /data

# Copy published files
COPY --from=publish /app/publish .

# Set environment variables
ENV ASPNETCORE_URLS=http://+:80
ENV ASPNETCORE_ENVIRONMENT=Production
ENV ConnectionStrings__DefaultConnection="Data Source=/data/worklog.db"

# Expose port
EXPOSE 80

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -f http://localhost/health || exit 1

# Run as non-root user
USER app

# Start the application
ENTRYPOINT ["dotnet", "WorkLog.Website.dll"]
