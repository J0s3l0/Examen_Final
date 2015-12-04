/*
Examen Final Problema - Busqueda de Caminos para Wally
José Luis García Reymundo
A01063645
*/


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <time.h>
#include <omp.h>
#include <signal.h>

#define N 4 
#define P 4 
#define NO 2          //num obstaculos
#define RUTAS 10000   //rutas maximas

typedef struct{
  int id;
  int i;
  int j;
  int ia;   //anterior en x
  int ja;   //anterior en y
  int n;    //niveles
  int iz;   //cuadrante izquierdo
  int der;  //cuadrante derecho
  int arr;  //cuadrante de arriba
  int aba;  //cuadrante de abajo
  int ini;  //inicio del rango
  int fin;  //fin del rango
}cuadrante;

cuadrante * array; //arreglo de todos los cuadrantes en el tablero

int * tablero;
pthread_mutex_t m_tablero = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m_caminos = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t m_lock = PTHREAD_MUTEX_INITIALIZER;

char * ruta;    
char ** rutas;  

int total_caminos = 0;
int cuadrados;
int num_secciones;

int calculasecciones();
int calcula();
void * mover(void *);
void * manejador (void *);


int main(int argc, char** argv) {

  int x, y, i, j;

  pthread_t sig;
  sigset_t set;

  int final;

  sigemptyset(&set);
  sigaddset(&set, SIGUSR1);
  sigaddset(&set, SIGINT);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  cuadrados = calcula();
  num_secciones = calculasecciones();

  pthread_t cuadrantes[P];
  array = (cuadrante *) malloc(sizeof(cuadrante) * P);
  cuadrante * auxarray = array;
  cuadrante * finalarray = array + P;

  for(j = 0; auxarray < finalarray; ++auxarray, ++j){
    
    auxarray->id  = j;
    auxarray->ia  = 0;
    auxarray->ja  = 0;
    auxarray->i   = 0;
    auxarray->j   = 0;
    auxarray->n   = 0;
    auxarray->aba = 0;
    auxarray->arr = 0;
    auxarray->iz  = 0;
    auxarray->der = 0;

    if(j = 0){
      
      auxarray->ini = 0;
      auxarray->fin = (cuadrados -1);
      final = (cuadrados - 1);

    }else{

      auxarray->ini = (final + 1);
      auxarray->fin = final + cuadrados;
      final += cuadrados;

    }
  }

  pthread_mutex_init(&m_tablero, NULL);
  auxarray = array;
  tablero = (int *) malloc(sizeof(int) * N * N);
  int * auxtablero = tablero;
  int * finalaux = tablero + (N * N);
  rutas = (char **) malloc(sizeof(char *) * RUTAS);
  char ** auxrutas = rutas;
  char ** finalrutas = rutas + RUTAS;
  ruta = (char *) malloc(sizeof(char) * 5 * N * N);

  srand(time(0));

  #pragma omp parallel
  {

    #pragma omp for
    for(auxrutas = rutas; auxrutas < finalrutas; ++auxrutas){

      *(auxrutas) = (char *) malloc(sizeof(char) * 5 * N * N);

    }

    #pragma omp for
    for(auxtablero = tablero; auxtablero < finalaux; ++auxtablero){

      *(auxtablero) = 0;

    }

  }

  //Obstaculos
  for(i = 0; i < NO; ++i){
    
    x = rand() % N;
    y = rand() % N;
    *(tablero + (y * N) + x) = 1;
    
  }

  for(j = 0; j < P; ++j, ++auxarray){

    pthread_create(&cuadrantes[j], NULL, mover, (void *)(auxarray));

  }

  pthread_create(&sig,NULL, manejador, NULL);   
  auxtablero = tablero;

  auxrutas=rutas;
    
  for(j = 0; j < P; ++j){

    pthread_join(cuadrantes[j], NULL);

  }

  sleep(10);
  pthread_kill(sig, SIGUSR1);
  
  auxrutas=rutas;

  for(;auxrutas < rutas; ++rutas){

    free(*(auxrutas));

  }
         
  free(tablero);
  free(rutas);
  free(ruta);

  return 0;
}

int calculasecciones(){
  int s = (log (P))/ log(2);
  return s;
}

int calcula(){
  int c = N / (calculasecciones());
  return c;
}


void * mover(void * arr) {
   
  sigset_t set;
  sigemptyset(&set);

  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGUSR1);
  
  cuadrante * arreglo = (cuadrante *)arr;
  cuadrante c = *(arreglo);

  int id = 0, i = 0, j = 0, n = 0, ia = 0, ja = 0, fin = 0, ini = 0;
  int m_ini;
  id  = c.id;
  i   = c.i;
  j   = c.j;
  n   = c.n;
  ia  = c.ia;
  ja  = c.ja;
  fin = c.fin;
  ini = c.ini;
   
  cuadrante aux = *(array + id);
  m_ini = aux.ini;

  if(ini != m_ini){

    pthread_mutex_lock(&m_lock);  
    pthread_cond_broadcast(&cond);
    pthread_cond_wait(&cond, &m_lock);
    pthread_mutex_unlock(&m_lock);

  }

  //No hay nada en la izquierda
  if(id % num_secciones == 0) arreglo->iz = -1;

  //No hay nada en la derecha
  if((id+1) % num_secciones == 0) arreglo->der = -1;

  if(id < num_secciones) arreglo->arr= -1;
    
  //Fuera del límite
  if(i >= N  || j >= N || j < 0 || i < 0) return ;

  //Obstaculo en el camino
  if(*(tablero + (j * N) + i) == 1){
    
    return ;

  } else {

    pthread_mutex_lock(&m_tablero);
    *(tablero + (j * N) + i) = 1; 
    pthread_mutex_unlock(&m_tablero);

  }

  //Nueva ruta
  if (i == N-1 && j== N-1) {

    pthread_mutex_lock(&m_tablero);

    *(tablero + (j * N) + i) = 0;
    sprintf(ruta + n * 7,"(%d, %d)", i, j);
    sprintf(*(rutas + caminos), ruta);

    pthread_mutex_lock(&m_caminos);
    ++caminos;
    pthread_mutex_unlock(&m_caminos);

    pthread_mutex_unlock(&m_tablero);

    pthread_cond_broadcast(&cond);
    
    return ;

  } else{

    #pragma omp task
    sprintf(ruta + n * 5,"(%d, %d)->", i, j);
    
    if(i+1 != ia){

      arreglo->i = i+1;
      arreglo->n = n+1;
      mover((void*)arreglo);

    }

    if(j+1 != ja){

      arreglo->j=j+1;
      arreglo->n=n+1;
      mover((void*)arreglo);

    }

    if(i-1 != ia){

      arreglo->i = i-1;
      arreglo->n = n+1;
      mover((void*)arreglo);

    }

    if(j-1 != ja){

      arreglo->j = j-1;
      arreglo->n = n+1;
      mover((void*)arreglo);

    }

    pthread_mutex_lock(&m_tablero);
    
    *(tablero + (j * N) + i) = 0;
    pthread_mutex_unlock(&m_tablero);

    pthread_cond_broadcast(&cond);

  }

}

void * manejador (void * n){
    
  sigset_t set;
  int sig, i;
  
  sigemptyset(&set);

  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGUSR1);

  while(1){
    
    sigwait(&set, &sig);
      
    if(sig == SIGINT || sig == SIGUSR1){

      pthread_mutex_lock(&m_caminos);
      printf("Hay %d caminos posibles  \n", total_caminos);
      
      for(i = 0; i < total_caminos; ++i){

        printf("%s\n", *(rutas + i));

      }

      pthread_mutex_unlock(&m_caminos);
      
    }
  }
}

