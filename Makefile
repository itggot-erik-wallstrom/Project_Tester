build:
	gcc ./main.c -o project_tester `pkg-config --cflags --libs gtk+-3.0 vte-2.91` 
