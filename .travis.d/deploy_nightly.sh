#!/bin/bash
API_URL="https://api.github.com/repos/$TRAVIS_REPO_SLUG/releases/$GH_NIGHTLY_ID"
UPLOAD_URL="https://uploads.github.com/repos/$TRAVIS_REPO_SLUG/releases/$GH_NIGHTLY_ID"

if [ "$TRAVIS_PULL_REQUEST" != "false" ]
then
    echo "This commit is pull request. No deploy!"
    exit 0
fi

if [ "$TRAVIS_BRANCH" != "master" ]
then
    echo "This commit was made against the $TRAVIS_BRANCH and not the master! No deploy!"
    exit 0
fi

REV=$(git rev-parse --short HEAD)
BUILD_TIME=$(date --rfc-3339=seconds)

echo "target branch:revision - $TRAVIS_BRANCH:$REV"
echo "built $BUILD_TIME"

# remove tags
git remote add upstream "https://$GH_TOKEN@github.com/$TRAVIS_REPO_SLUG.git" > /dev/null 2>&1
git push -q upstream ":refs/tags/nightly" > /dev/null 2>&1
git tag -d nightly > /dev/null 2>&1
git tag nightly > /dev/null 2>&1
git push -q upstream --tags > /dev/null 2>&1
git tag

# delete pre uploaded
ASSECT_URL=$(curl -s -H "Authorization: token $GH_TOKEN" "$API_URL/assets" | \
             python $TRAVIS_BUILD_DIR/.travis.d/asset_id.py)

if [ "$(echo $ASSECT_URL | xargs | wc -l)" -ne "0" ]
then
    curl -s -X DELETE -H "Authorization: token $GH_TOKEN" $ASSECT_URL
else
    echo "not exists"
fi
# upload
curl -s -X POST \
     -H "Authorization: token $GH_TOKEN" \
     -H "Content-Type: application/zip" \
     --data-binary @$TRAVIS_BUILD_DIR/build/savemgr.vpk \
     "$UPLOAD_URL/assets?name=savemgr.vpk"

# update release note
curl -s -X PATCH \
     -H "Authorization: token $GH_TOKEN" \
     -H "Content-Type: application/json" \
     -d "{\"tag_name\":\"nightly\",\"target_commitish\":\"$TRAVIS_BRANCH\",\"name\":\"Nightly build - $BUILD_TIME\",\"body\":\"[CHANGELOG](CHANGELOG.md#unreleased)\",\"draft\":false,\"prerelease\":false}" \
     "$API_URL"
