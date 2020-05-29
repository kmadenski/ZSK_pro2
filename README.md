# ZSK_pro2
Projekt zaliczeniowy na potrzeby realizacji przedmiotu - **Zaawansowane systemy operacyjne (ZSO)**

## Podstawowe informacje

**Messanger** - pojedynczy proces. Może być automatyczny lub manualny w odniesieniu do treści zadania.
## Parametryzacja
W pliku ***run*** jest możliwość określenia ile będzie uruchomionych procesów automatycznych. W pliku ***main.cpp*** jest określona maksymalna liczba procesów jakie będą obsługiwane przez pamięć dzieloną z uwagi na realizację pamięci dzielonej przy użycia tablic statycznych. 
### Opis parametrów:

    //Maksymalna liczba procesów. Powinna być to suma procesów, które będą realizowały funkcję automatycznych messangerów oraz manualnych.
    #define MAX_PROCESSES 10  

    //Maksymalna liczba wiadomości po których otrzymaniu proces automatyczny zostanie zakończony
    #define MAX_MESSAGES 3  

    //Opóźnienie pojedynczego cyklu wątku wysyłającego wiadomości w ramach procesu
    #define WRITER_DELAY 3  

    //Opóźnienie pojedynczego cyklu wątku odbierającego wiadomości w ramach procesu
    #define READER_DELAY 1  

    //Opóźnienie startu procesu - Tylko w przypadku messangerów automatycznych
    #define START_DELAY 2  

## Przykładowe uruchomienie

Uruchomienie messangerów automatycznych. Uwzględnia buildowanie.

    //Messangery automatyczne zostaną uruchomione w tle, a ich rezultat będzie dostępny w pliku wynikowym
    ./run > result

Uruchomienie messangera manualnego

    ./main m
