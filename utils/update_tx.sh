#!/bin/sh

PROJECT_NAME=ytbtocue
PROJECT_ORGANIZATION=qmmp-development-team
TX_CONFIG="../.tx/config"

echo "[main]" > ${TX_CONFIG}
echo "host = https://www.transifex.com" >> ${TX_CONFIG}
echo "" >> ${TX_CONFIG}

plug_name=${PROJECT_NAME}

echo "Updating ${plug_name}"

echo "[o:${PROJECT_ORGANIZATION}:p:${PROJECT_NAME}:r:${plug_name}]" >> ${TX_CONFIG}
echo "file_filter = src/translations/${plug_name}_<lang>.ts" >> ${TX_CONFIG}
echo "source_lang = en" >> ${TX_CONFIG}
echo "source_file = src/translations/${plug_name}_en.ts" >> ${TX_CONFIG}
echo "type = QT" >> ${TX_CONFIG}
echo "" >> ${TX_CONFIG}


RESOURCE_NAME=${PROJECT_NAME}

echo "Updating ${RESOURCE_NAME}-desktop"

echo "[o:${PROJECT_ORGANIZATION}:p:${PROJECT_NAME}:r:${RESOURCE_NAME}-desktop]" >> ${TX_CONFIG}
echo "file_filter = src/desktop-translations/${RESOURCE_NAME}_<lang>.desktop.in" >> ${TX_CONFIG}
echo "source_lang = en" >> ${TX_CONFIG}
echo "source_file = src/${RESOURCE_NAME}.desktop" >> ${TX_CONFIG}
echo "type = DESKTOP" >> ${TX_CONFIG}
echo "" >> ${TX_CONFIG}
