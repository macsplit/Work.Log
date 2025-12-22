#!/bin/bash

# Password Reset Script for Work.Log Web App

DB_PATH="/home/user/.local/share/WorkLog/Work Log/worklog.db"

# Check if database exists
if [ ! -f "$DB_PATH" ]; then
    echo "Error: Database not found at $DB_PATH"
    exit 1
fi

# Get username
echo "Available users:"
sqlite3 "$DB_PATH" "SELECT Id, Username FROM Users;"
echo ""
read -p "Enter username to reset password for: " USERNAME

# Check if user exists
USER_EXISTS=$(sqlite3 "$DB_PATH" "SELECT COUNT(*) FROM Users WHERE Username = '$USERNAME';")
if [ "$USER_EXISTS" -eq 0 ]; then
    echo "Error: User '$USERNAME' not found"
    exit 1
fi

# Get new password
read -sp "Enter new password: " PASSWORD
echo ""
read -sp "Confirm new password: " PASSWORD_CONFIRM
echo ""

if [ "$PASSWORD" != "$PASSWORD_CONFIRM" ]; then
    echo "Error: Passwords do not match"
    exit 1
fi

if [ -z "$PASSWORD" ]; then
    echo "Error: Password cannot be empty"
    exit 1
fi

# Generate BCrypt hash using dotnet
cd "$(dirname "$0")/Domain"

HASH=$(dotnet run --project . -- hash "$PASSWORD" 2>/dev/null)

if [ -z "$HASH" ]; then
    # Fallback: create a temporary console app to generate hash
    TEMP_DIR=$(mktemp -d)
    cd "$TEMP_DIR"

    dotnet new console -n HashGen --force > /dev/null 2>&1
    cd HashGen
    dotnet add package BCrypt.Net-Next > /dev/null 2>&1

    cat > Program.cs << 'EOF'
var password = args.Length > 0 ? args[0] : "";
Console.WriteLine(BCrypt.Net.BCrypt.HashPassword(password, BCrypt.Net.BCrypt.GenerateSalt(12)));
EOF

    HASH=$(dotnet run -- "$PASSWORD" 2>/dev/null)

    # Cleanup
    cd /
    rm -rf "$TEMP_DIR"
fi

if [ -z "$HASH" ]; then
    echo "Error: Failed to generate password hash"
    exit 1
fi

# Update the database
sqlite3 "$DB_PATH" "UPDATE Users SET PasswordHash = '$HASH' WHERE Username = '$USERNAME';"

if [ $? -eq 0 ]; then
    echo "Password successfully reset for user '$USERNAME'"
else
    echo "Error: Failed to update password in database"
    exit 1
fi
