#include<stdio.h>

int main() {
    int nro_club, cont_temp, cont_perm, edad, cod;
    for (nro_club = 1; nro_club <= 17; nro_club++){
        printf("Ingrese el codigo del socio (0 para terminar): ");
        scanf("%d", &cod);
        cont_temp = 0;
        cont_perm = 0;
        while (cod != 0){ 
            printf("Ingrese la edad del socio: ");
            scanf("%d", &edad);
            if (edad > 17){
                if(cod==1){
                    cont_temp++;
                } else {
                    cont_perm++;
                }
            }
            printf("Ingrese el codigo del socio (0 para terminar): ");
            scanf("%d", &cod);
        }
        if (cont_temp > cont_perm){
            printf("El club %d tiene mas socios temporales que permanentes.\n", nro_club);
        } else if (cont_perm > cont_temp){
            printf("El club %d tiene mas socios permanentes que temporales.\n", nro_club);
        } else {
            printf("El club %d tiene igual cantidad de socios temporales y permanentes.\n", nro_club);
        }
    }
    return 0;
}