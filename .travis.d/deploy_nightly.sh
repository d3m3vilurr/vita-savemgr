#!/bin/bash
API_URL="https://api.github.com/repos/$TRAVIS_REPO_SLUG/releases/$GH_NIGHTLY_ID"
BUILD_TIME=$(date --rfc-3339=seconds)

# update release note
curl -s -X PATCH \
     -H "Authorization: token $GH_TOKEN" \
     -H "Content-Type: application/json" \
     -d "{\"tag_name\":\"nightly\",\"target_commitish\":\"$TRAVIS_BRANCH\",\"name\":\"Nightly $TRAVIS_BUILD_NUMBER - $BUILD_TIME\",\"body\":\"[CHANGELOG](CHANGELOG.md#unreleased)\",\"draft\":false,\"prerelease\":false}" \
     "$API_URL"
