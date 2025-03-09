#!/bin/bash

# Liczba połączeń
NUM_CONNECTIONS=100

# Adres URL
URL="http://localhost/test"

# Funkcja do wykonywania pojedynczego połączenia
make_request() {
    START_TIME=$(date +%s%N)  # Początek pomiaru czasu w nanosekundach

    # Wykonanie żądania i zapisanie kodu statusu HTTP
    HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" "$URL")

    END_TIME=$(date +%s%N)  # Koniec pomiaru czasu w nanosekundach

    # Obliczenie czasu trwania w milisekundach
    DURATION=$(( (END_TIME - START_TIME) / 1000000 ))

    # Wyświetlenie wyniku
    echo "Status: $HTTP_CODE, Czas: $DURATION ms"
}

# Początek pomiaru całkowitego czasu
TOTAL_START_TIME=$(date +%s%N)

# Wykonanie 100 równoległych połączeń
for ((i = 1; i <= NUM_CONNECTIONS; i++)); do
    make_request &
done

# Oczekiwanie na zakończenie wszystkich połączeń
wait

# Koniec pomiaru całkowitego czasu
TOTAL_END_TIME=$(date +%s%N)

# Obliczenie całkowitego czasu w milisekundach
TOTAL_DURATION=$(( (TOTAL_END_TIME - TOTAL_START_TIME) / 1000000 ))

# Wyświetlenie podsumowania
echo "Wykonano $NUM_CONNECTIONS połączeń w $TOTAL_DURATION ms"