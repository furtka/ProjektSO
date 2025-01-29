## Ul
Rój pszczół liczy początkowo N osobników. W danym momencie czasu w ulu może znajdować się co
najwyżej P (P<N/2) pszczół. Pszczoła, która chce dostać się do wnętrza ula, musi przejść przez jedno
z dwóch istniejących wejść. Wejścia te są bardzo wąskie, więc możliwy jest w nich jedynie ruch w
jedną stronę w danej chwili czasu. Zbyt długotrwałe przebywanie w ulu grozi przegrzaniem, dlatego
każda z pszczół instynktownie opuszcza ul po pewnym skończonym czasie Ti. Znajdująca się w ulu
królowa, co pewien czas Tk składa jaja, z których wylęgają się nowe pszczoły. Aby jaja zostały złożone
przez królową, w ulu musi istnieć wystarczająca ilość miejsca. (1 jajo liczone jest tak jak 1 dorosły
osobnik).
Pszczelarz może dołożyć (sygnał1) dodatkowe ramki do ula, które pozwalają zwiększyć liczbę
pszczół maksymalnie do 2*N osobników. Może również wyjąć (sygnał2) ramki ograniczając bieżącą
maksymalną liczbę osobników o 50%.
Napisz program pszczelarza, pszczoły “robotnicy” i królowej, tak by zasymulować cykl życia roju
pszczół. Każda z pszczół “robotnic” umiera po pewnym określonym czasie Xi, liczonym ilością
odwiedzin w ulu.

# Program Hive
## Parametry programu:
N - liczba pszczół
P - max pszczół w ulu  ( P < N/2)
T_i - ciąg czasów po którym i-ta pszczoła chce opuścic ul
X_i - liczba odwiedzin w ulu po którym i-ta robotnica umiera
T - interwał, co jaki królowa składa jaja

## Sposób działania 
1. Program tworzy określoną liczbę procesów dzieci - workers & queen
2. Program pilnuje ile pszczół jest wewnątrz
3. program pilnuje, zasobu "wejście do ula" tak by nie więcej niz jedna pszczola mogła z niego skorzystac
    - program nasłuchuje w osobnym wątku komunikatów w kolejce - chcę wejść/wyjść 
       dodawanych od pszczół. jezeli dana akcja jest mozliwa, wysyla komunikat do 
       kolejki dla danej robotnicy, ze moze wykonac zadana akcje.
    - W momencie wyjścia / wejscia z/do ula, pszczola wraca z komunikatem o 
       zwolnieniu przejscia
    - W czasie gdy pszczola przchodzi przez dane przejscie, na przejscie jest zakladany lock (opcjonalnie)
    - W pierwszej kolejnosci obsługiwane są requesty wyjścia
4. nasluchuje sygnałów od pszczelarza i podwaja lub zminiejsza maksymalną liczbę pszczół w ulu
5. nasluchuje zapytan od pszczelarza o stan ula

# Program bee 
## Stan robotnicy:
1. OUTSIDE
2. WAIT_IN
3. INSIDE
4. WAIT_OUT

## Sposób działania
1. Robotnica chcąc wejść lub wyjść z ula, wysyła odpowiedni komunikat na kolejkę
   komunikatów do ula. 
2. Nastepnie nsłuchuje eventu kierowanego do niej, pozwalajacego
   na wejsćie lub wyjście
3. W momencie skonczenia tranzycji, wysyła komunikat o zwolnieniu
   wejścia.

w przypadku zmiany stanu na "na zewnątrz" lub "w środku" proces pszczoly "zasypia" na określoną ilość czasu (T_i, T_i)

# Program Queen


# TODO
[ ] implement queen and making new bees!
[ ] better error handling everywhere1
[ ] implement beekeeper
[ ] handle seamphores the linux way
[ ] validate input
[ ] document the project

## Optional
[ ] handle the compiler warning generated in the logger
[ ] handle log level