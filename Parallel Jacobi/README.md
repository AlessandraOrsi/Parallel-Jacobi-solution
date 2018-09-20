# A simple Jacobi iteration

## Indice
- Problem statement
- Soluzione proposta
- Risultati ottenuti

### Problem statement
Lo scopo di tale progetto è quello di implementare l'algoritmo di Jacobi per l'approssimazione della soluzione dei sistemi di equazioni lineari. In particolare, in questa implementazione, si vuole risolvere l'equazione di Laplace in due dimensioni ed approssimare la soluzione attraverso una computazione parallela.

### Soluzione proposta
La soluzione da me proposta prevede che ciascun processo abbia il proprio carico di lavoro sul quale effettuare la computazione. 
  ##### Distribuzione del carico
Ogni processo, compreso il master, crea la propria sottomatrice su cui      lavorare. Al termine della computazione ogni processo farà la GaterV al Master della propria sottomatrice affinchè tutti i valori interni delle sottomatrici vengano inseriti in una matrice finale di dimensione N.
E' stato necessario utilizzare la GatherV perchè le varie sottomatrici possono avere dimensione diversa l'una dall'altra a seconda del bilanciamento del carico tra i vari processi.
La sottomatrice per ciascun processo viene allocata dinamicamente e sotto forma di vettore.

           x_processi = (double *)malloc(sizeof(double) * l * n);

La stessa allocazione viene fatta per la matrice finale, quindi anch'essa sotto forma di vettore.
    La motivazione è la seguente: poichè MPI si aspetta di leggere la memoria in maniera contigua, in particolare la operazione di GatherV  (e la Gather in generale), è risultato necessario utilizzare come struttura dati il vettore.
    La dimensione delle sottomatrici, quindi la dimensione del carico di lavoro per ciascun processo, viene calcolata sia per il caso in cui la dimensione della matrice finale di dimensione N non è divisibile per il numero di processi, e sia per il caso in cui N è divisibile per il numero di processi.
    
                int works[size];
              	int resto=n%size;
              	int l; //lavoro
              	int disp[size]; //displacement
              	int dis=0;
              	
              	for(i=0;i<size;i++){
              		l = n/size;
              		if(resto>0){
              			works[i]=(l+1)*n;
              			resto--;
              		}
              		else{ 
              			works[i]=l*n;
              		}
              		disp[i]=dis;
              		dis+=works[i];
              	}
              	
              	l = works[rank]/n;
              	l = l + 2;
  
  Ogni sottomatrice, oltre le righe interne su cui vengono effettuati i calcoli, possiede due righe in più: 
  -- una che rappresenta il bound superiore, che tiene traccia (attraverso la comunicazione) dell'ultima riga effettiva della sottomatrice precedente (appartenente ad un altro processo);
  -- l'altra rappresenta il bound inferiore, che tiene traccia (attraverso la comunicazione) della prima riga effettiva della sottomatrice successiva (appartenente ad un altro processo).
N.B. Per definizione, il primo processo avrà come prima riga effettiva una riga di soli '-1' e che non viene mai modificata, e allo stesso modo l'ultimo processo avrà come ultima riga effettiva una riga di soli '-1' che rimarrà inalterata. 
Ovviamente il primo processo non farà uso del bound superiore perchè non ha una sottomatrice precedente, e allo stesso modo l'ultimo processo non farà uso del bound inferiore perchè non ha una sottomatrice successiva.

La computazione sulle sottomatrici procede fino a quando il numero di iterazioni è < 100 e la tolleranza è > 1.0e-2. Il numero di iterazioni viene aggiornato attraverso un semplice contatore, mentre la tolleranza viene aggiornata ad ogni iterazione da ciscun processo e attraverso una MPI_Allreduce vengono combinati tutti i valori della tolleranza di tutti i processi e distribuiti i risultati aggiornati a tutti i processi.

##### Istruzioni per la compilazione e l'esecuzione
Compilazione: mpicc jacobi.c -o jacobi
Esecuzione: mpirun -np #processi --host MASTER,IP_SLAVE1,IP_SLAVE2,... jacobi 

### Risultati ottenuti
La tipologia di istanza su cui ho dovuto effettuare i test è la m4.xlarge:

vCPUs | Memory (GiB) | Network Performance 
------|--------------|--------------------
4     |      16      |     High

##### Strong Scalability
All'interno della Strong Scalability, la dimensione del problema rimane fissata e utilizzata da tutti i processi. Lo scopo è quello di minimizzare il tempo di esecuzione del programma.
Nel mio caso, ho fissato la dimensione della matrice a N = 14000 in modo tale da raggiungere più di 3 minuti di computazione nel caso di una istanza e un core.
 
Number of Instances |	Number of Cores	 |    Time (sec)
--------------------| -------------------| --------------
         1	        |          1	     |     324,801356
         1	        |          4	     |     141,033191
         2  	    |          8	     |      75,671007
         3	        |          12	     |      55,925771
         4	        |          16	     |      46,408473
         5	        |          20	     |      39,436262
         6	        |          24	     |      35,669434
         7	        |          28	     |      33,162350
         8	        |          32	     |      31,043284

[![Strong Scalability](https://s16.postimg.org/af9e7apb9/Strong_Scalability.png
 "Strong scalability graph")](https://s16.postimg.org/af9e7apb9/Strong_Scalability.png)

Come è possibile osservare nel grafico, coesite una proporzionalità inversa tra numero di processori e tempo, quindi all'aumentare del numero di processori, il tempo diminuisce. Da notare come il tempo si dimezza nel passare da 4 a 8 processori.

##### Weak Scalability
All'interno della Weak Scalability, la dimensione del problema cambia (aumentando) da processo a processo. Lo scopo è quello di risolvere problemi molto grandi.
Nel mio caso, ad ogni processo ho assegnato:
1. Numero di righe = (14000/32) * numero di core utilizzati
2. Numero di colonne = 14000

				

Matrix: Number of Rows | Matrix: Number of Columns |Number of Instances |Number of Cores|Time(sec)
-----|--------|-----|----|-----------
438 |	14000 |	1 |	1  |	10,053816
1750|	14000|	1 |	4  |  	17,923382
3500|	14000|	2 |	8  |	19,574520
5250|	14000|	3|	12 |	21,137268
7000|	14000|	4|	16 |	23,139931
8750|	14000|	5|	20 |	24,359190
10500|	14000|	6|	24 |	26,543935
12250|	14000|	7|	28 |	29,334160
14000|	14000|	8|	32 |	31,447282

[![Weak Scalability](https://s16.postimg.org/nxgajl1gl/Weak_Scalability.png
 "Weak scalability graph")](https://s16.postimg.org/nxgajl1gl/Weak_Scalability.png)

Come è possibile notare nel grafico, coesiste una proporzionalità diretta tra numero di processori e tempo, in relazione all'aumentare della taglia del problema per processo.
    