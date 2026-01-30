#!/usr/bin/env bash

TAG="tlCleanupEpilog"

if [ -z "$SLURM_JOB_ID" ]; then
    logger -t $TAG "$TAG: Error - SLURM_JOB_ID is not set. Cannot check job name. Exiting."
    exit 1
fi

if [ -z "$SLURM_JOB_USER" ]; then
    logger -t $TAG "$TAG: Error - SLURM_SLURM_JOB_USER not set. Exiting."
    exit 1
fi

USER_HOME=$(getent passwd "$SLURM_JOB_USER" | cut -d: -f6)
if [ -z "$USER_HOME" ]; then
    logger -t $TAG "$TAG: Error - Could not find home directory for user $SLURM_JOB_USER. Exiting."
    exit 1
fi

SECRET_FILE_TO_DELETE="$USER_HOME/.thinlinc/.ood-secrets/job-$SLURM_JOB_ID"

if [ -f "$SECRET_FILE_TO_DELETE" ]; then
    logger -t $TAG "$TAG: Found ThinLinc secret for Job $SLURM_JOB_ID. Proceeding with cleanup."

    rm -f "$SECRET_FILE_TO_DELETE"
    logger -t $TAG "$TAG: Removed secret hash: $SECRET_FILE_TO_DELETE Status: $?"

    JOB_CONFIG_TO_DELETE="$USER_HOME/.thinlinc/.ood-secrets/session_config.$(hostname)"
    if [ -f "$JOB_CONFIG_TO_DELETE" ]; then
        rm -f "$JOB_CONFIG_TO_DELETE"
        logger -t $TAG "$TAG: Removed session config: $JOB_CONFIG_TO_DELETE. Status: $?"
    fi

else
    logger -t $TAG "$TAG: Job $SLURM_JOB_ID does not appear to be a ThinLinc job (no secret file found). Skipping."
    exit 0
fi
