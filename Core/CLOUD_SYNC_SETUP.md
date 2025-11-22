# Work.Log Cloud Sync Setup Guide

This guide explains how to set up Amazon DynamoDB for syncing your Work.Log data across devices.

## Prerequisites

- An AWS account
- AWS CLI installed (optional, for command-line setup)

## Step 1: Create an IAM User

1. Go to the [AWS IAM Console](https://console.aws.amazon.com/iam/)
2. Click **Users** in the left sidebar
3. Click **Create user**
4. Enter a username (e.g., `worklog-sync`)
5. Click **Next**
6. Select **Attach policies directly**
7. Search for and select `AmazonDynamoDBFullAccess` (or create a custom policy - see below)
8. Click **Next**, then **Create user**
9. Click on the created user
10. Go to **Security credentials** tab
11. Click **Create access key**
12. Select **Application running outside AWS**
13. Click **Next**, then **Create access key**
14. **Save both the Access Key ID and Secret Access Key** - you'll need these for configuration

### Optional: Create a Restricted Policy

For better security, create a custom policy that only allows access to the Work.Log tables:

```json
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Action": [
                "dynamodb:Query",
                "dynamodb:PutItem",
                "dynamodb:GetItem",
                "dynamodb:UpdateItem",
                "dynamodb:DeleteItem",
                "dynamodb:DescribeTable"
            ],
            "Resource": [
                "arn:aws:dynamodb:*:*:table/WorkLog_Sessions",
                "arn:aws:dynamodb:*:*:table/WorkLog_Tags"
            ]
        }
    ]
}
```

## Step 2: Create DynamoDB Tables

### Using AWS Console

#### Create Sessions Table

1. Go to the [DynamoDB Console](https://console.aws.amazon.com/dynamodb/)
2. Click **Create table**
3. Configure:
   - **Table name**: `WorkLog_Sessions`
   - **Partition key**: `ProfileId` (String)
   - **Sort key**: `CloudId` (String)
4. Under **Table settings**, select **Customize settings** if you want to use On-demand capacity (recommended for personal use)
5. Click **Create table**

#### Create Tags Table

1. Click **Create table** again
2. Configure:
   - **Table name**: `WorkLog_Tags`
   - **Partition key**: `ProfileId` (String)
   - **Sort key**: `CloudId` (String)
3. Configure the same settings as above
4. Click **Create table**

### Using AWS CLI

```bash
# Create Sessions table
aws dynamodb create-table \
    --table-name WorkLog_Sessions \
    --attribute-definitions \
        AttributeName=ProfileId,AttributeType=S \
        AttributeName=CloudId,AttributeType=S \
    --key-schema \
        AttributeName=ProfileId,KeyType=HASH \
        AttributeName=CloudId,KeyType=RANGE \
    --billing-mode PAY_PER_REQUEST \
    --region us-east-1

# Create Tags table
aws dynamodb create-table \
    --table-name WorkLog_Tags \
    --attribute-definitions \
        AttributeName=ProfileId,AttributeType=S \
        AttributeName=CloudId,AttributeType=S \
    --key-schema \
        AttributeName=ProfileId,KeyType=HASH \
        AttributeName=CloudId,KeyType=RANGE \
    --billing-mode PAY_PER_REQUEST \
    --region us-east-1
```

Replace `us-east-1` with your preferred AWS region.

## Step 3: Configure Work.Log

### Desktop App (KDE/Kirigami)

1. Open Work.Log
2. Click the menu button (three dots) in the top-right
3. Select **Cloud Sync**
4. Click **Configure Sync** (or **Edit Configuration** if already configured)
5. Enter:
   - **Profile ID**: A unique identifier for your data (e.g., `john-worklog`)
   - **AWS Access Key ID**: From Step 1
   - **AWS Secret Access Key**: From Step 1
   - **AWS Region**: The region where you created the tables
6. Click **Save Configuration**
7. Click **Test Connection** to verify
8. Click **Sync Now** to perform your first sync

### Web App (.NET)

1. Log in to Work.Log
2. Click **Sync** in the navigation
3. Click **Configure Sync**
4. Enter the same credentials as above
5. Click **Save Configuration**
6. Click **Sync Now**

## Data Schema

### Sessions Table Structure

| Attribute | Type | Description |
|-----------|------|-------------|
| ProfileId | String | Partition key - identifies the user/profile |
| CloudId | String | Sort key - UUID for each session |
| SessionDate | String | ISO date (YYYY-MM-DD) |
| TimeHours | Number | Hours worked |
| Description | String | Session description |
| Notes | String | Optional notes |
| NextPlannedStage | String | Optional next steps |
| TagCloudId | String | Reference to tag's CloudId |
| CreatedAt | String | ISO timestamp |
| UpdatedAt | String | ISO timestamp (used for conflict resolution) |
| IsDeleted | Boolean | Soft delete flag |

### Tags Table Structure

| Attribute | Type | Description |
|-----------|------|-------------|
| ProfileId | String | Partition key - identifies the user/profile |
| CloudId | String | Sort key - UUID for each tag |
| Name | String | Tag name |
| UpdatedAt | String | ISO timestamp (used for conflict resolution) |
| IsDeleted | Boolean | Soft delete flag |

## Sync Behavior

- **Conflict Resolution**: Uses `UpdatedAt` timestamp - the most recent change wins
- **Soft Deletes**: Deleted items are marked with `IsDeleted=true` to sync deletions across devices
- **Tag References**: Sessions reference tags via `TagCloudId` rather than local IDs

## Cost Estimation

With On-demand capacity mode:
- **Free Tier**: 25 GB storage, 25 read/write capacity units (enough for most personal use)
- **Beyond Free Tier**: ~$0.25 per million read requests, ~$1.25 per million write requests

For typical personal use (a few syncs per day), costs should be minimal or free.

## Troubleshooting

### "Connection failed" error
- Verify your AWS credentials are correct
- Check that the table names match (`WorkLog_Sessions` and `WorkLog_Tags`)
- Ensure your IAM user has the necessary permissions

### "Table not found" error
- Verify the tables were created in the correct AWS region
- Check that the table names are exactly `WorkLog_Sessions` and `WorkLog_Tags`

### Data not syncing between devices
- Ensure both devices are using the same **Profile ID**
- Check that both devices have recent data by comparing `UpdatedAt` timestamps
- Try clicking "Sync Now" on both devices

## Security Notes

- AWS credentials are stored locally on each device
- Credentials are never transmitted except to AWS for authentication
- Use the restricted IAM policy for better security
- Consider enabling MFA on your AWS account
