wait_for_http_code()
{
    local WAIT_URL=$1
    local WAIT_CODE=$2
    local TOKEN=$3
    echo "wait_for_http_code($WAIT_URL, $WAIT_CODE)"

    local CODE=""
    local MAX_RETRIES=20
    local RETRIES=0
    while [[ (-z "$CODE" || "$CODE" -ne "$WAIT_CODE") && ("$RETRIES" -lt "$MAX_RETRIES") ]]
    do
      sleep 1
      CODE=$(curl --silent --header "Authorization: token $TOKEN" --head "$WAIT_URL" | grep ^HTTP | tr -s ' ' | cut -f 2 -d' ')
      RETRIES=`expr $RETRIES + 1`
      echo "DEBUG. Waiting for $WAIT_URL = $WAIT_CODE. Retries $RETRIES, code $CODE"
    done
    echo "DEBUG. Waiting finished. Retries $RETRIES, code $CODE"
}

upload_zip_file()
{
  local UPLOAD_URL=$1
  local FILE=$2
  local TOKEN=$3
  echo "upload_zip_file($UPLOAD_URL, $FILE)"

  # Limit rate is used to ensure uploads finish
  # When not used, systematic errors for TCP connection reset are received
  curl \
      --retry 20 --retry-delay 0 --retry-max-time 40 --max-time 180 --limit-rate 5M \
      --request POST \
      --url $UPLOAD_URL?name=$FILE \
      --header "Authorization: token $TOKEN" \
      --header "Content-Type: application/zip" \
      --data-binary @$FILE
}

download_asset() {
  local GH_API_REPO=$1
  local RELEASE_TAG=$2
  local ASSET_NAME=$3
  local TOKEN=$4
  local ASSET_URL=$($CURL_JSON $GH_API_REPO/releases/tags/$RELEASE_TAG | python3 -c \
    "import sys, json; assets = [a for a in json.load(sys.stdin)['assets'] if a['name'] == '$ASSET_NAME']; print(assets[0]['browser_download_url']) if assets else ''" \
    )
  echo "INFO. asset url:"
  echo "$ASSET_URL"
  echo ""
  if [ ! -z $ASSET_URL ]; then
    download_file $ASSET_URL $ASSET_NAME $TOKEN
  else
    echo "Asset $ASSET_NAME not found in $RELEASE_TAG release in repository $REPO"
    exit 1
  fi
}

download_file()
{
  local DOWNLOAD_URL=$1
  local FILENAME=$2
  local TOKEN=$3
  curl \
      --retry 20 --retry-delay 0 --retry-max-time 40 --max-time 180 --limit-rate 5M \
      --remote-header-name \
      --location \
      --header "Authorization: token $TOKEN" \
      --header "Accept: application/octet-stream" \
      --url "$DOWNLOAD_URL" \
      --create-dirs \
      --output "$FILENAME"
}
