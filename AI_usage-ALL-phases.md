# Curs: Sisteme de Operare (SO / SO1)

## Proiect: Sistem Centralizat de Monitorizare și Raportare a Infrastructurii Urbane

### Autor: Trifut Alexandra Rebecca


## 1. Instrumentul Utilizat

În cadrul tuturor celor trei faze de dezvoltare ale proiectului, instrumentul de Inteligență Artificială utilizat a fost Google Gemini. Acesta a fost întrebuințat ca un partener de dialog tehnic, fiind utilizat pentru generarea de structuri repetitive (boilerplate code), clarificarea standardelor POSIX și depanarea erorilor de sincronizare (debugging).

## 2. Faza 1: Gestiunea Fișierelor, Permisiuni Programatice și Filtrare Dinamică
   A. Promptele Oferite (Context și Specificitate)

Pentru a automatiza logica de parsare a inputului din linia de comandă, am furnizat un prompt extrem de specific, bazat pe constrângerile binare ale proiectului:

    "Am o structură Report în C definită astfel: int id; char name[32]; double lat, lon; char category[20]; int severity; time_t timestamp; char description[128];. Te rog să îmi generezi o funcție robustă numită parse_condition(const char *input, char *field, char *op, char *value) care serializează/sparge un șir primit de tipul 'field:operator:value' în cele 3 componente folosind delimitatori. De asemenea, generează o funcție match_condition(Report *r, const char *field, const char *op, const char *value) care primește o înregistrare citită de pe disc și returnează 1 dacă aceasta respectă criteriul, realizând conversiile de tip necesare din text în int sau time_t."

B. Ce a fost Generat de către IA

IA a livrat cod sursă funcțional axat pe manipularea string-urilor:

    Pentru parse_condition, a folosit o mască avansată de citire în formatul specifice familiei scanf: sscanf(input, "%[^:]:%[^:]:%s", field, op, value).

    Pentru match_condition, a generat o structură arborescentă extensivă de decizie (if-else imbricate), grupate pe tipuri de date (comparare numerică directă pentru severity și timestamp, respectiv comparare lexicografică prin strcmp pentru string-urile category și inspector).

C. Modificări Aplicate și Justificarea Inginerească

Deși funcțiile parititare de evaluare logică funcționau corect izolat, IA a rezolvat problema exclusiv în memoria volatilă (RAM). Schimbările mele esențiale au fost de natură arhitecturală:

    Am conceput de la zero logica de I/O în city_manager.c. Am utilizat apelurile primitive de sistem open(..., O_RDONLY) și am creat o buclă de citire iterativă while(read(fd, &r, sizeof(Report)) > 0).

    Am implementat un parser pentru argumentele multiple din argv[] pentru a procesa filtre multiple conjugate logic prin operatorul general AND. Astfel, funcțiile IA au fost folosite doar ca o "sită" internă de filtrare a fluxului de octeți extrași de mine de pe disc.

D. Concepte Însușite

    Expresii Regulate în sscanf: Am înțeles modul în care clasa de caractere %[^:] funcționează ca un delimitator puternic, eliminând necesitatea unor procesări manuale greoaie cu strtok sau strchr.

    Izolarea Bitmask-urilor: Am învățat că pentru extragerea și validarea corectă a drepturilor de acces într-un mod strict legat de rol (Manager vs Inspector), nu trebuie comparat întreg cuvântul de mod (st_mode), ci trebuie aplicat un complement binar și operații de AND bit cu bit cu macro-urile S_IRUSR, S_IWGRP etc.

## 3. Faza 2: Managementul Proceselor și Semnalizarea Asincronă (POSIX Signals)
   A. Metodologia de Utilizare a IA

În această etapă, codul a fost generat în proporție majoritară manual, IA fiind utilizat pe post de consultant teoretic pentru migrarea de la funcțiile învechite ANSI C la standardele moderne POSIX Compliant.
B. Concepte Clarificate și Corecții Implementate

    Fluxul Fork-Exec-Wait: Am utilizat IA pentru a vizualiza exact ce se întâmplă cu spațiul de adrese în momentul apelului remove_district. Am fost ghidată să folosesc fork() pentru a izola execuția distructivă a utilitarului rm -rf, mapând parametrii în mod securizat în cadrul familiei de apeluri execlp și blocând părintele cu wait(NULL) pentru a preveni crearea de procese orfane sau Zombie.

    Modernizarea Managementului de Semnale: Inițial, codul tindea spre utilizarea funcției primitive signal(). IA a oferit documentație critică privind comportamentul nesigur al acesteia (cum ar fi resetarea handler-ului pe anumite platforme UNIX). Cu ajutorul său, am implementat structurile struct sigaction, configurând corect masca de blocare prin sigemptyset() și setările atomice ale flag-urilor pentru handlerele SIGUSR1 și SIGINT.

    Tratarea Defensivă a Erorilor (Defensive Programming): AI-ul m-a ajutat să implementez un mecanism stabil în city_manager.c la adăugarea unui raport. În loc ca eșecul trimiterii semnalului kill(m_pid, SIGUSR1) să prăbușească execuția, am interceptat codurile de eroare pentru a asigura scrierea corectă în logged_district a stării monitorului (dacă acesta era activ, inactiv sau dacă fișierul .monitor_pid lipsea).

## 4. Faza 3: Canale de Comunicație Multiplexate (Pipes) și Redirecționări Complexe de Flux
   A. Promptele Oferite (Probleme de Sincronizare și Buffering)

În cadrul ultimei faze, cea mai mare provocare a fost interconectarea programului city_hub cu procesele independente monitor_reports și scorer. Am utilizat următoarele interogări:

        "Cum pot clona un descriptor de fișier dintr-un pipe astfel încât stdout-ul unui proces apelat prin execl să devină intrarea părintelui, fără a bloca terminalul?"

        "De ce în momentul în care adaug un raport din city_manager, mesajul trimis de monitor nu apare în timp real în interfața din city_hub, deși pipe-ul este deschis?"

B. Soluțiile Oferite de IA

    Mecanismul dup2: IA mi-a explicat cum tabela de descriptori de fișiere din kernel a procesului copil este rescrisă prin apelul dup2(fd[1], STDOUT_FILENO). În momentul în care procesul execută execl, noul program moștenește descriptorul clonat, scriind direct în canalul de comunicație.

    Misterul Stream Buffering-ului: IA a identificat cauza principală a lipsei mesajelor instantanee: când stdout este conectat la un terminal, este "Line-Buffered" (trimite datele la fiecare \n), dar când este redirecționat către un Pipe, devine automat "Block-Buffered" (așteaptă acumularea a 4096 de octeți). Soluția a fost adăugarea macro-ului fflush(stdout).

C. Refactorizările și Contribuția Personală

Logica distribuită din city_hub.c a fost o provocare structurală personală. Am implementat o arhitectură de Dublu Fork:

    Primul fork() izolează procesul background numit hub_mon, împiedicând blocarea CLI-ului principal din Hub.

    Al doilea fork() lansează monitorul nativ.

Am adaptat codul sugerat de IA pentru a gestiona citirea continuă: am adăugat o buclă de procesare while(read(...) > 0) în interiorul hub_mon pentru a intercepta mesajele asincrone în orice moment, păstrând în același timp controlul promptului hub>  deschis pentru utilizator. De asemenea, am mapat dinamic argumentele variabile primite în calculate_scores pentru a aloca pipe-uri dedicate fiecărui proces scorer rulat secvențial.

## 5. Concluzii și Evaluare Personală a Procesului de Învățare

Proiectul a reprezentat o evoluție clară în înțelegerea programării la nivel de sistem (System-Level Programming). Colaborarea cu Google Gemini a evidențiat următoarele aspecte fundamentale:

    Eficiență în Automatizare: Modelele de limbaj sunt instrumente excepționale pentru scrierea rapidă de cod algoritmic redundant (mecanismele de filtrare, logica repetitivă de calcul matematic, structurile repetitive).

    Necesitatea Expertizei Umane: IA nu are o viziune de ansamblu asupra stării sistemului de operare. Gestionarea corectă a descriptorilor (închiderea capetelor neutilizate din pipe-uri close(fd[1]) pentru a genera semnalul de EOF), evitarea blocajelor de tip Deadlock și persistența corectă a structurilor binare pe disc sunt în întregime responsabilitatea arhitecturală a programatorului.

Fără o înțelegere profundă a apelurilor de sistem POSIX, codul generat parțial de IA ar fi rămas o colecție de funcții izolate, incapabile să ruleze într-un mediu de execuție concurent și distribuit.
