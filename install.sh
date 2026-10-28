#!/usr/bin/env bash
# Установка cryptum: файлы копируются в /opt/cryptum, ссылка создаётся в /usr/local/bin (уже есть в PATH).
# Запуск: sudo ./install.sh
set -euo pipefail

if [[ $EUID -ne 0 ]]; then
    echo "Запустите установку с правами администратора: sudo ./install.sh" >&2
    exit 1
fi

SOURCE="$(cd "$(dirname "$0")" && pwd)/bin"
TARGET=/opt/cryptum
LINK=/usr/local/bin/cryptum

if [[ ! -x "$SOURCE/cryptum" ]]; then
    echo "Сначала соберите проект: make" >&2
    exit 1
fi

install -d "$TARGET"
install -m 755 "$SOURCE/cryptum" "$TARGET/cryptum"
install -m 644 "$SOURCE"/lib*.so "$TARGET/"
ln -sf "$TARGET/cryptum" "$LINK"

echo "Программа установлена в $TARGET, запуск: cryptum"
