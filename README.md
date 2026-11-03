# Proiect Sisteme de Operare (SO) - Management și Monitorizare Procese

## 📌 Despre Proiect
Acest proiect este o aplicație dezvoltată în limbajul **C**, concepută pentru a simula gestionarea și monitorizarea unor rapoarte și procese regionale, utilizând concepte avansate de sisteme de operare.

## 🛠️ Concepte și Tehnologii folosite
* **Limbaj:** C
* **Sistem de operare:** Linux / UNIX
* **Mecanisme IPC:** Pipe-uri pentru comunicarea între procese
* **Sisteme de fișiere:** Gestionare de symlinks și fișiere de rapoarte

## 📂 Structura Proiectului
* `city_hub.c` - Modulul central de coordonare.
* `city_manager.c` - Gestionează fluxul de date și procesele pentru orașe/regiuni.
* `monitor_reports.c` - Monitorizează și centralizează rapoartele primite.
* `scorer.c` - Evaluează și calculează scorurile pe baza datelor colectate.
* `Arad/` & `Timis/` - Directoare de lucru cu fișiere specifice regiunilor.

## 🚀 Cum se rulează
Pentru a compila fișierele sursă, poți folosi comanda `gcc` în terminal:
```bash
gcc city_hub.c -o city_hub
gcc city_manager.c -o city_manager
