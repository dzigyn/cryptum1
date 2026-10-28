#!/usr/bin/env bash
# Удаление cryptum. Запуск: sudo ./uninstall.sh
set -euo pipefail

if [[ $EUID -ne 0 ]]; then
    echo "Запустите удаление с правами администратора: sudo ./uninstall.sh" >&2
    exit 1
fi

rm -f /usr/local/bin/cryptum
rm -rf /opt/cryptum
echo "Программа cryptum удалена."
