# ZemaxDDEClient

<a id="ru"></a>

>**🌐 Документация доступна на нескольких языках:**
[![EN](https://img.shields.io/badge/%F0%9F%87%AC%F0%9F%87%A7%20EN-inactive?style=flat-square)](README.md#en)
[![RU](https://img.shields.io/badge/%F0%9F%87%B7%F0%9F%87%BA%20RU-blue?style=flat-square)](#ru)

[![Latest Release](https://img.shields.io/github/v/release/d3m37r4/ZemaxDDEClient?include_prereleases&style=flat-square&color=blue)](https://github.com/d3m37r4/ZemaxDDEClient/releases)
[![Windows](https://img.shields.io/badge/platform-Windows-0078d7.svg?style=flat-square)](https://en.wikipedia.org/wiki/Microsoft_Windows)
[![Build Test](https://img.shields.io/github/actions/workflow/status/d3m37r4/ZemaxDDEClient/.github%2Fworkflows%2Fbuild-test.yml?branch=main&style=flat-square&label=build%20test)](https://github.com/d3m37r4/ZemaxDDEClient/actions/workflows/build-test.yml)
[![License](https://img.shields.io/github/license/d3m37r4/ZemaxDDEClient?style=flat-square)](https://github.com/d3m37r4/ZemaxDDEClient/blob/main/LICENSE)

## 📌 О проекте
Приложение для работы с [Zemax](https://en.wikipedia.org/wiki/Zemax), обеспечивающее расширенный анализ оптических систем за счёт прямого доступа к их параметрам через DDE (Dynamic Data Exchange).

<details>
<summary><b>Работа с Zemax через DDE</b></summary>

### Что такое DDE?
Dynamic Data Exchange (DDE) — это протокол межпроцессного взаимодействия в Windows, позволяющий обмениваться данными между приложениями. Два приложения могут установить DDE-соединение:
- **Сервер** (в данном случае — Zemax) — предоставляет данные,
- **Клиент** (ваше приложение) — запрашивает и получает данные.

Zemax реализует интерфейс DDE-сервера, что даёт внешним приложениям доступ к данным оптических систем и функциональности программы.

### Как работает ZemaxDDEClient
Данный проект является DDE-клиентом, который:
1. Устанавливает DDE-соединение с Zemax в качестве сервера,
2. Отправляет запросы на получение данных (например, параметров оптической системы),
3. Получает и обрабатывает ответы от Zemax.

## Справочная документация
Полный список команд DDE см. в разделе **«Глава 28: РАСШИРЕНИЯ ZEMAX»** официальной документации Zemax.
</details>

## 🔽 Скачать
Готовые сборки для Windows доступны на странице [Releases](https://github.com/d3m37r4/ZemaxDDEClient/releases).

- [**Релизные сборки**](https://github.com/d3m37r4/ZemaxDDEClient/releases) — стабильные, протестированные версии.  
- [**Dev-сборки**](https://github.com/d3m37r4/ZemaxDDEClient/actions) — тестовые версии с новым или экспериментальным функционалом, ещё не включённым в релиз (могут быть нестабильны).

> 💡 Каждая сборка содержит две версии:<br>
> – **Release** — для обычного использования,<br>
> – **Debug** — с консольным выводом и расширенным логированием для отладки.<br>
> Обе имеют одинаковый номер билда для удобства сопоставления.

## 🏁 Запуск
Приложение можно запустить двумя способами: через меню **Extensions** в Zemax или напрямую, запустив исполняемый файл.

### Вариант 1: Запуск через меню Extensions в Zemax
Этот способ интегрирует приложение в меню расширений Zemax для удобного доступа.
1. Запустите **Zemax**  
2. Откройте меню Extensions (по умолчанию — горячая клавиша `F11`)
3. В списке выберите исполняемый файл вида: `ZemaxDDEClient_*.exe`
> 💡 Zemax сканирует папку `Extend` в своей директории установки (обычно `C:\Program Files\Zemax\Extend\`) и отображает все `.exe`-файлы из неё в меню **Extensions**. Поместите туда ваш исполняемый файл, чтобы он стал доступен постоянно.

### Вариант 2: Прямой запуск
Вы можете запустить исполняемый файл напрямую, без добавления в Zemax.
1. Запустите **Zemax**  
2. Запустите файл `ZemaxDDEClient_*.exe` 
3. Нажмите кнопку **Connect to Zemax** в окне **Sidebar**

## 📦 Требования
- **MSYS2** с набором инструментов MinGW-w64  
- **CMake** (версия ≥ 3.16)  
- **GLFW**: `pacman -S mingw-w64-x86_64-glfw`  
- **Zemax** (работает как DDE-сервер после запуска)

## 📚 Сторонние библиотеки
Этот проект использует следующие сторонние библиотеки:
- **[Dear ImGui](https://github.com/ocornut/imgui)** от Omar Cornut — библиотека для создания графических интерфейсов (Immediate Mode)
- **[ImPlot](https://github.com/epezent/implot)** от Evan Pezent — библиотека двумерной визуализации для Dear ImGui
- **[Native File Dialog (NFD)](https://github.com/mlabbe/nativefiledialog)** от Michael Labbe — кроссплатформенные нативные диалоги выбора файлов и папок

## 🚀 Подготовка к сборке
1. Склонируйте репозиторий:
   ```bash
   git clone --recurse-submodules https://github.com/d3m37r4/ZemaxDDEClient.git
   cd ZemaxDDEClient
   ```
2. Установите MSYS2 и необходимые зависимости:
   ```bash
   pacman -Syu
   pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-glfw mingw-w64-x86_64-pkg-config git zip
   ```
3. Загрузите подмодули (если клонирование выполнялось без флага --recurse-submodules):
   ```bash
   git submodule update --init --recursive
   ```

## 🔨 Сборка
Проект использует CMake для надёжной сборки. Для удобства рекомендуется использовать скрипт-обёртку `build.sh`.
   ```bash
   # Назначьте файлу build.sh права на выполнение
   chmod +x build.sh

   # Соберите релизную версию (используется по умолчанию)
   ./build.sh

   # Соберите отладочную версию (с видимой консолью и включённым DEBUG_LOG)
   ./build.sh debug

   # Сборка с оптимизацией: -O2
   ./build.sh optimize=1

   # Сборка с максимальной оптимизацией: -O3
   ./build.sh optimize=2

   # Debug-сборка с оптимизацией -O2 (пример комбинирования параметров)
   ./build.sh debug optimize=1

   # Очистка сборочной директории перед сборкой
   ./build.sh clean

   # Сборка с фиксированным временем — для стабильных артефактов в CI и автоматизации
   BUILD_TIMESTAMP=1766073737 ./build.sh release
   ```

Ручная сборка через CMake (если требуется тонкая настройка или отладка процесса сборки):
   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ```

## 📄 Лицензия
Проект распространяется под лицензией [MIT](https://github.com/d3m37r4/ZemaxDDEClient/blob/main/LICENSE).

## 🤝 Вклад и поддержка
Если у вас есть идеи, сообщения об ошибках или предложения по улучшению проекта, пожалуйста, напишите мне одним из способов ниже:

[![GitHub Issues](https://img.shields.io/badge/GitHub-Issues-blue?logo=github&style=flat-square)](https://github.com/d3m37r4/ZemaxDDEClient/issues)  
[![GitHub Discussions](https://img.shields.io/badge/GitHub-Discussions-blue?logo=github&style=flat-square)](https://github.com/d3m37r4/ZemaxDDEClient/discussions)  
[![Telegram](https://img.shields.io/badge/Telegram-dmitry__isakow-blue?logo=telegram&style=flat-square)](https://t.me/dmitry_isakow)

Вы также можете отправить [pull request](https://github.com/d3m37r4/ZemaxDDEClient/pulls).
