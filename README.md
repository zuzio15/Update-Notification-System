# Update Notification System

## Opis

Aplikacja działająca w tle w systemie Windows, która:

- działa bez okna konsoli
- tworzy ikonę w System Tray
- ma menu kontekstowe z opcją „Wyjście”
- monitoruje pliki logów aplikacji
- wykrywa linie zawierające odpowiednią frazę
- wyświetla powiadomienie w oknie Raylib

---

## Funkcje

- Ikona w zasobniku systemowym (Tray)
- Obsługa kliknięcia PPM i menu
- Monitorowanie odpowiedniego pliku:
- Wykrywanie tekstu np: `update started`
- Powiadomienie Raylib (okno 300x120)
- Automatyczne zamknięcie po ~5 sekundach
- Działanie w osobnym wątku

---

## Struktura

- `main.cpp` → Win32 + tray + logika aplikacji  
- `notification.cpp` → Raylib popup  
- `notification.h` → deklaracja TriggerNotification()

---

## Użyto

- C++17+
- Win32 API
- Raylib


---

## Uruchomienie

1. Zbuduj projekt w CLion / CMake
2. Uruchom aplikację
3. Ikona pojawi się w System Tray
4. W razie wykrycia odpowiedniego wpisu pojawi się okno Raylib
