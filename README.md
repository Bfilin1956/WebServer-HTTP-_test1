# Многопоточный веб-сервер Boost::Asio (Асинхронный)

## Описание
Реализован многопоточный веб-сервер на C++, который обрабатывает HTTP-запросы. Сервер минималистичен и поддерживает базовые функции:

- Прием соединений от нескольких клиентов одновременно.
- Обработка HTTP GET-запросов к статическим файлам.
- Логирование активности сервера (запросов).

## Возможности

1. **Многопоточность**: Сервер может обслуживать несколько клиентов одновременно.
2. **Обработка GET-запросов**: Сервер ищет запрашиваемый файл в указанной директории и возвращает его содержимое. Если файл не найден, возвращается ошибка 404.
3. **Логирование**: Запросы клиентов (метод, путь и IP-адрес) сохраняются в лог-файле `server.log` с временной меткой.

```
Проект/
|-- WWWROOT/       # Каталог для статических файлов
|   |-- index.html # Пример файла
|   |-- style.css  # Пример файла
|-- server.log     # Лог-файл (создается автоматически)
|-- ConsoleApplication1.cpp       # Исходный код
```
![ezgif-2-4bfa452146](https://github.com/user-attachments/assets/8cd171a1-2b63-4552-a0c1-daff9b743b23)
---
![Снимок экрана 2024-12-30 232148](https://github.com/user-attachments/assets/1fa71cd7-2eb3-4a03-90c3-440d5a0d76f8)
