#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "pizzapizza.h"
#include <errno.h>
#include <sys/wait.h>
/*
  Authors:   Iason Chatzopoulos                3150197
*/
Const_times* Create_const_times_argument(struct Init_file_data file_data){
  //pairnoume mono ta aparaithta dedomena 
  Const_times* argument = malloc(sizeof(Const_times));
  argument->t_prep=file_data.t_prep;
  argument->t_bake=file_data.t_bake;
  argument->t_pack=file_data.t_pack;
  argument->t_del_low=file_data.del_low;
  argument->t_del_high=file_data.del_high;
  return argument;
}
double Calculate_average_wait_time(int num_of_clients){
  return global_data.total_wait_time/num_of_clients;
}
double Calculate_average_throughput_time(int num_of_clients){
  return global_data.total_throughput_time/num_of_clients;
}
int calculate_random_value(int min, int max){
  return rand()%(max-min+1)+min;;
}

void Print_Stats(struct Argument_data data, struct Init_file_data file_data){
  printf("----Total_pizzas sold: %d\n",global_data.total_income/10);
  printf("----Total_income:      %d\n\n",global_data.total_income);
  printf("-- -Average client wait_time in minutes: %lf -- -\n",Calculate_average_wait_time(data.clients_num)/600);
  printf("-- -Average client throughput time in minutes: %lf -- -\n",Calculate_average_throughput_time(data.clients_num)/600);
  printf("Average Cold Wait time: %f\n",global_data.average_wait_time/data.clients_num);
  printf("Average Service time: %f\n",global_data.average_service_time/data.clients_num);
  printf("Maximum service time: %d\n", global_data.maximum_wait_time);
  printf("Maximum cold pizza wait time: %d\n", global_data.maximum_service_time);
}

//==MUTEX/Cond-FUNCTIONS==//
void Lock_on_mutex(pthread_mutex_t *mutex) {

  if (pthread_mutex_lock(mutex)) {
    printf("locking on mutex failed\n");
    exit(-1);
  }
}
void Unlock_on_mutex(pthread_mutex_t* mutex){
  if(pthread_mutex_unlock(mutex)){
    printf("unlocking on mutex failed\n");
    exit(-1);
  }
}
void Wait_on_cond(pthread_cond_t* cond,pthread_mutex_t* mutex){
  
  if(pthread_cond_wait(cond,mutex)){
    printf("wait on condition failed\n");
    exit(-1);
  }
}

void simulate_wait_time(const thread_arg *data) {
  int seconds_to_sleep = calculate_random_value(data->t_order_low, data->t_order_high);
  sleep(seconds_to_sleep*0.0001);
}
int Check_if_credit_fails(double fail_probability){
  int random_number = calculate_random_value(0,100);
  // if the card fails return 1 else return 0
  if((double)random_number/100>fail_probability)
    return 0;
  return 1;
}

void Update_total_income(int transaction_cost){
  Lock_on_mutex(&mutexes_and_cond.Update_income);
  global_data.total_income +=transaction_cost;
  Unlock_on_mutex(&mutexes_and_cond.Update_income);
}

void Update_total_transactions(){
  Lock_on_mutex(&mutexes_and_cond.Update_transaction_counter);
  global_data.total_transactions++;
  Unlock_on_mutex(&mutexes_and_cond.Update_transaction_counter);
}

double Calc_time_passed(struct timespec start,struct timespec end){
  int long diff_in_ms;
  diff_in_ms=(end.tv_sec-start.tv_sec)*100000 +(end.tv_nsec-start.tv_nsec)/1000;
  return diff_in_ms;
}

Server_return_data* Serve_client(thread_arg* data, int num_of_pizzas){
  //pairnei orisma ta thread arg kai ton arithmo pizzwn
  //ftiaxnome to transaction info kai kanoume elegxo gia to an eggrinetai h oxi 
  // h paraggelia pernontas tis katallhles times ana periptwsh. 
  Server_return_data* transaction_info = malloc(sizeof(Server_return_data));
  transaction_info->pizzas=num_of_pizzas;
  simulate_wait_time(data);
  printf("|---Order number: %d wants %d  pizzas \n",data->order_id,num_of_pizzas);
  transaction_info->wait_time = calculate_random_value(data->t_payment_low,data->t_payment_high);
  sleep(transaction_info->wait_time*0.0001);
  if(Check_if_credit_fails(data->p_fail)){

    transaction_info->error_num++;
    printf("~-==Order #%d transaction failed!==-~\n",data->order_id);
    transaction_info->flag=1;
    
   global_data.num_of_fails++;
    return transaction_info;
    
  }  
  // transaction_info->seat_list=Book_Seats(num_of_pizzas,data->client_id);
  transaction_info->flag=0;
  transaction_info->transaction_cost = num_of_pizzas*data->c_pizza;
  
  Update_total_income(transaction_info->transaction_cost);
  transaction_info->transaction_id=(global_data.total_transactions+1);
  Update_total_transactions();
  
  return transaction_info;
}

//==Thread-functions==//
void* thread(void *arg){
  //cast ta dedomena 
  thread_arg* data = (thread_arg*)arg;
  //nanoseconds
  struct timespec wait_start,wait_end,throughput_start,throughput_end;

  //rologia gia throughput_time;
  clock_gettime(CLOCK_REALTIME,&throughput_start);
  //wait_time
  clock_gettime(CLOCK_REALTIME,&wait_start);
  
  //printf("\ndeutero lock");
  //an den brei thlefona perimenei mexri na skasei to signal 
  int order_id_  = data->order_id;
  if(order_id_){
   int seconds_to_sleep = calculate_random_value(data->t_order_low, data->t_order_high);
    global_data.total_wait_time=+(global_data.total_wait_time + seconds_to_sleep);    
    sleep(seconds_to_sleep/100);
  }
  
  Lock_on_mutex(&mutexes_and_cond.Available_telephone);
  clock_gettime(CLOCK_REALTIME,&wait_end);
  while (global_data.telephones_available==0){
    Wait_on_cond(&mutexes_and_cond.Telephone_cond,&mutexes_and_cond.Available_telephone);
  }
  //an perasei to loop shmainei oti exoume toulaxiston 1 thlefoniti opote meiwnoume kata 1
  global_data.telephones_available--;
  Unlock_on_mutex(&mutexes_and_cond.Available_telephone);
  clock_gettime(CLOCK_REALTIME,&wait_end);
  //pairnoume arithmo pizzwn me vash ta orismata
  int num_of_pizzas = calculate_random_value(data->n_order_low, data->n_order_high);
  //tsekarei an petuxainei to transaction. se fail bgazei katallilo minima kai kanei log tis pitses 
  //an failarei profanws den proxoraei
  //xrisimopoioume ena flag sto struct transaction_info kai tsekaroume apla an einai 0
  Server_return_data* transaction_info = Serve_client(data,num_of_pizzas);

  if(transaction_info->flag==1){
     pthread_cond_signal(&mutexes_and_cond.Telephone_cond);  
    global_data.telephones_available++;
    free(transaction_info);
    free(data);
    global_data.total_wait_time=+Calc_time_passed(wait_start,wait_end);
    pthread_exit(NULL);
  }
  //telos tou order ksekinaei h proetoimasia apo dw kai pera
  //apodesmevetai  to thl kai paei sthn epomenh paraggelia
  pthread_cond_signal(&mutexes_and_cond.Telephone_cond);  
  global_data.telephones_available++;
  //printf("\ntelephones : %d \n",global_data.telephones_available );
  
  //preping:
  printf("|---Received an order with number: %d.\n",data->order_id);
  Lock_on_mutex(&mutexes_and_cond.Available_cooks);
  //printf("|---Order number: %d wants %d  pizzas \n",data->order_id,num_of_pizzas);
  while(global_data.cooks_available==0){
    Wait_on_cond(&mutexes_and_cond.Cooks_cond, &mutexes_and_cond.Available_cooks);
  }
  global_data.cooks_available--;
  //perimenei prep_time*arithmo pizzwn tou order. 
  //sto sleep kanei kai kala thn proetoimasia
  //desmevei kathe fora tous mageires gia to xrono twn pizzwn kai molis 
  //teleiwsei oles tis pitses tis paraggelias apodesmevetai k stelnetai
  // gia psisimo 
  //printf("Order number: %d is being prepared!   \n",data->order_id);
  sleep(data->const_arg.t_prep*num_of_pizzas*0.0001);
  Unlock_on_mutex(&mutexes_and_cond.Available_cooks);
  pthread_cond_signal(&mutexes_and_cond.Cooks_cond);
  global_data.cooks_available++;
  //perimenei na uparksoun toulaxiston osoi fournoi einai 
  //kai oi paraggelies . molis ginoun eleutheroi oi fournoi,
  // tous desmevei kai meta to psismo/ sleep tous ksekleidwnei
  // kai auksanei to available ovens
  Lock_on_mutex(&mutexes_and_cond.Available_ovens);
  while(num_of_pizzas > global_data.ovens_available){
    Wait_on_cond(&mutexes_and_cond.Ovens_cond, &mutexes_and_cond.Available_ovens);
  }
  for(int i = 0; i< num_of_pizzas; i++){
    global_data.ovens_available--;
  }
  //printf("Order number: %d is getting baked!\n",data->order_id);
  sleep(data->const_arg.t_bake*num_of_pizzas*0.0001);
  Unlock_on_mutex(&mutexes_and_cond.Available_ovens);
  pthread_cond_signal(&mutexes_and_cond.Ovens_cond);
  for(int i = 0; i< num_of_pizzas; i++){
    global_data.ovens_available++;
  }
  //ena atomo paketarei omoiws me prin oso einai desmevmenos oi alloi perimenoun
  Lock_on_mutex(&mutexes_and_cond.Available_packet_guy);
  while(global_data.packet_guy==0){
    Wait_on_cond(&mutexes_and_cond.Packet_guy_cond, &mutexes_and_cond.Available_packet_guy);
  }
  global_data.packet_guy--;
  printf("|==-Order number: %d Preparing pizzas took %d minutes.==-\n",data->order_id, (data->const_arg.t_pack+data->const_arg.t_prep)/60*num_of_pizzas+data->const_arg.t_bake/60);
  sleep(data->const_arg.t_pack*num_of_pizzas*0.0001);

  Unlock_on_mutex(&mutexes_and_cond.Available_packet_guy);
  global_data.packet_guy++;
  pthread_cond_signal(&mutexes_and_cond.Packet_guy_cond);
  
  //deli guy omoiws me ta parapanw apla analambanei ena
  // order ti fora aneksartita to poses pitses exei
  Lock_on_mutex(&mutexes_and_cond.Available_deli_guys);
  while(global_data.deliguys_available==0){
    Wait_on_cond(&mutexes_and_cond.Deli_guys_cond, &mutexes_and_cond.Available_deli_guys);
  }
  global_data.deliguys_available--;
  int deli_time= calculate_random_value(data->const_arg.t_del_low,data->const_arg.t_del_high);
  printf("|-=-Order number: %d is on its way! estimated arrival in: %d minutes.-=-\n",data->order_id,deli_time/60);
  sleep(deli_time*0.0001);
  //printf("order number: %d is returning.   \n",data->order_id);
  sleep(deli_time*0.0001);
  //printf("order number: %d delivery is ready to be deployed.   \n",data->order_id);
  Unlock_on_mutex(&mutexes_and_cond.Available_deli_guys);
  global_data.deliguys_available++;
  pthread_cond_signal(&mutexes_and_cond.Deli_guys_cond);
  printf("|=-=Order number: %d is completed! Pizza(s) were delivered in %d minutes since ordering!=-=\n",data->order_id,deli_time/60+(data->const_arg.t_bake+data->const_arg.t_pack+data->const_arg.t_prep)/60*num_of_pizzas+transaction_info->wait_time/60 );
  //stop clock for throughput time
  clock_gettime(CLOCK_REALTIME,&throughput_end);

  //update total throughput_time;
  Lock_on_mutex(&mutexes_and_cond.Update_wait_time);
  global_data.total_throughput_time=+Calc_time_passed(throughput_start,throughput_end);
  Unlock_on_mutex(&mutexes_and_cond.Update_wait_time);

  //update total wait_time;
  Lock_on_mutex(&mutexes_and_cond.Update_wait_time);
  global_data.total_wait_time=+Calc_time_passed(wait_start,wait_end);
  Unlock_on_mutex(&mutexes_and_cond.Update_wait_time);
  //sunolikos xronos diekperewshs paragkelias se lepta  (bake-delivery)
  int tsk = deli_time/60+(data->const_arg.t_bake+data->const_arg.t_pack+data->const_arg.t_prep)/60*num_of_pizzas+transaction_info->wait_time/60;
  //sunolikos xronos diekperewshs paragkelias se lepta   
  int tsk2 = deli_time/60+data->const_arg.t_pack/60*num_of_pizzas+transaction_info->wait_time/60;
  global_data.average_service_time+=tsk;
  global_data.average_wait_time+=tsk2;
  if(tsk>=global_data.maximum_wait_time){
    global_data.maximum_wait_time=tsk;
  }
  if(tsk2>=global_data.maximum_service_time){
    global_data.maximum_service_time=tsk2;
  }

  Lock_on_mutex(&mutexes_and_cond.Writing_in_stdout);
  Unlock_on_mutex(&mutexes_and_cond.Writing_in_stdout);
  free(data);
  free(transaction_info);
  pthread_exit(NULL);
  
}

thread_arg* Create_thread_argument(int order_id,struct Init_file_data file_data, struct Const_times time_args){
  //initilize ta thread arguments pou pernane sth methodo *thread
  thread_arg* argument = malloc(sizeof(thread_arg));
  argument->order_id=order_id;
  argument->t_order_low=file_data.t_order_low;
  argument->t_order_high=file_data.t_order_high;
  argument->t_payment_low=file_data.t_payment_low;
  argument->t_payment_high=file_data.t_payment_high;
  argument->n_order_low=file_data.n_order_low;
  argument->n_order_high=file_data.n_order_high;
  argument->p_fail=file_data.p_fail;
  argument->c_pizza=file_data.c_pizza;
  argument->const_arg.t_bake=time_args.t_bake;
  argument->const_arg.t_prep=time_args.t_prep;
  argument->const_arg.t_pack=time_args.t_pack;
  argument->const_arg.t_del_high=time_args.t_del_high;
  argument->const_arg.t_del_low=time_args.t_del_low;
  return argument;
}

void Start_Clients(pthread_t* clients_array,int num_of_clients, struct Init_file_data file_data){
//to struct const times einai mesa sto arguments kai periexei statheres gia ti leitourgia 
Const_times* const_times =Create_const_times_argument(file_data);
  for(int i =0;i<num_of_clients;i++){
    //gia kathe client ftiaxnoume tou pername orisma tin methodo *thread kai to struct const_times
    //edw trexoun oi psistes/mageires/thlefonites klp
    if(i){
      sleep(calculate_random_value(file_data.n_order_low,file_data.t_order_high)/1000);
    }
    thread_arg* arg = Create_thread_argument(i,file_data,*const_times);
    if(pthread_create(&(clients_array[i]),NULL,thread,(void*)arg)){
      printf("thread error");
      exit(-1);
    }
    
  }
  free(const_times);
  
}

void Wait_for_clients_to_finish(pthread_t* clients_array,int num_of_clients){
  for (int i = 0; i<num_of_clients ;i++) {
    if(pthread_join(clients_array[i],NULL)){
      printf("thread error");
      exit(-1);
    }
  }
}
// //==DELETING FUNCTIONS==//
void Destroy_Mutex(pthread_mutex_t* mutex){
  if(pthread_mutex_destroy(mutex)){
    printf("error while destroying mutex\n");
    exit(-1);
  }
}

void Destroy_cond(pthread_cond_t* cond){
  if(pthread_cond_destroy(cond)){
    printf("error while destroying cond\n");
    exit(-1);
  }
}

void Destroy_Mutexes_and_cond(){
  Destroy_Mutex(&mutexes_and_cond.Available_telephone);
  Destroy_Mutex(&mutexes_and_cond.Available_cooks);
  Destroy_Mutex(&mutexes_and_cond.Available_ovens);
  Destroy_Mutex(&mutexes_and_cond.Available_packet_guy);
  Destroy_Mutex(&mutexes_and_cond.Available_deli_guys);
  Destroy_Mutex(&mutexes_and_cond.Update_income);
  Destroy_Mutex(&mutexes_and_cond.Writing_in_stdout);
  Destroy_Mutex(&mutexes_and_cond.Update_wait_time);
  Destroy_Mutex(&mutexes_and_cond.Update_orders);
  Destroy_cond(&mutexes_and_cond.Telephone_cond);
  Destroy_cond(&mutexes_and_cond.Packet_guy_cond);
  Destroy_cond(&mutexes_and_cond.Ovens_cond);
  Destroy_cond(&mutexes_and_cond.Cooks_cond);
  Destroy_cond(&mutexes_and_cond.Deli_guys_cond);

}

void check_Arguments_number(int argc){

  if(argc!=5){
    printf("Wrong number of arguments\n");
    printf("Correct format is: -n 'clients_num' -r 'random seed'\n");
    exit(-1);
  }
}

Argument_data Get_Arguments(int argc,char** argv){
  //koitame gia ton arithmo pelatwn kai to seed 
  //typwnoume ta katallhla sfalmata
  check_Arguments_number(argc);
  int clients_num;
  int seed;

  for(int i =1;i<argc;i++){

    if(!strcmp("-n",argv[i])){
      i++;
      clients_num=atoi(argv[i]);
    }

    else if(!strcmp("-r",argv[i])){
      i++;
      seed=atoi(argv[i]);
    }

    else{
      printf("WRONG ARGUMET FORMAT\n");
      printf("Correct format is: -n 'clients_num' -r 'random seed\n");
      exit(-1);
    }
  }

  Argument_data data = {clients_num,seed};
  return data;
}

// FILE *Open_file() {
//   //ftiaxnoume ena pointer p deixnei sto arxeio , kanoume readonly kai epistrefei to pointer
//   FILE *Initialization_file = fopen("./init_variables", "r");
  
//   if (Initialization_file==NULL) {
//     printf("error at file opening");
//     printf("%s",strerror(errno));
//     fclose(Initialization_file);
//     exit(-1);
    
//   }
  
//   return Initialization_file;
// }

// Init_file_data Read_data_from_file(FILE * Initialization_file){   //FILE* Initialization_file as argument
//   //diabazoume apo to txt kai ta pername se ena struct typoy Init_file_data, 
//   int n_tel = 3;
//   int n_cook = 2;
//   int oven = 10;
//   int n_deliverer=7;
//   int t_order_low = 60;
//   int t_order_high= 300;
//   int n_order_low=60;
//   int n_order_high=300;
//   int t_payment_low =60;
//   int t_payment_high=120;
//   int c_pizza=10;
//   double p_fail=0.05;
//   int t_prep=60;
//   int t_bake=600;
//   int t_pack=120;
//   int del_low=300;
//   int del_high=900;
  
//   // //fscan(text...,arguments)kai ta fortwnei 
//   // fscanf(Initialization_file,"n_tel = %d\n"
//   //                             "n_cook = %d\n"
//   //                             "oven = %d\n"
//   //                             "n_deliverer = %d\n"
//   //                             "t_order_low = %d\n"
//   //                             "t_order_high = %d\n"
//   //                             "n_order_low = %d\n" 
//   //                             "n_order_high = %d\n"
//   //                             "t_payment_low = %d\n"
//   //                             "t_payment_high = %d\n"
//   //                             "c_pizza = %d\n"
//   //                             "p_fail = %lf\n"
//   //                             "t_prep = %d\n"
//   //                             "t_bake = %d\n"
//   //                             "t_pack = %d\n"
//   //                             "del_low = %d\n"
//   //                             "del_high = %d",&n_tel,&n_cook,&oven,&n_deliverer, &t_order_low,&t_order_high,&n_order_low,&n_order_high,
//   //                             &t_payment_low,&t_payment_high,&c_pizza,&p_fail,&t_prep,&t_bake,&t_pack,&del_low,&del_high);

//   Init_file_data file_data ={n_tel, n_cook, oven, n_deliverer, t_order_low, t_order_high,
//                              n_order_low,n_order_high,t_payment_low,t_payment_high,c_pizza,p_fail,
//                              t_prep,t_bake,t_pack,del_low,del_high};
//   //fclose(Initialization_file);
//   return file_data;
// }

Init_file_data Get_data_from_file(){

  //FILE* Initialization_file = Open_file();
  int n_tel = 3;
  int n_cook = 2;
  int oven = 10;
  int n_deliverer=7;
  int t_order_low = 60;
  int t_order_high= 300;
  int n_order_low=1;
  int n_order_high=5;
  int t_payment_low =60;
  int t_payment_high=120;
  int c_pizza=10;
  double p_fail=0.05;
  int t_prep=60;
  int t_bake=600;
  int t_pack=120;
  int del_low=300;
  int del_high=900;
  Init_file_data file_data ={n_tel, n_cook, oven, n_deliverer, t_order_low, t_order_high,
                             n_order_low,n_order_high,t_payment_low,t_payment_high,c_pizza,p_fail,
                             t_prep,t_bake,t_pack,del_low,del_high};
  return file_data;
}

void Init_mutex_and_check(pthread_mutex_t mutex){
  
  if(pthread_mutex_init(&mutex,NULL)){
    
    printf("Mutex init error");
    exit(-1);
  }
}

void Init_cond_and_check(pthread_cond_t cond){
  if(pthread_cond_init(&cond,NULL)){
    printf("condition initialization error");
    exit(-1);
  }  
}

void Initialize_mutexes(struct Mutexes_and_cond* mutexes_and_cond){
//ola ta mutexes ginontai initialize edw mazi me ta conditions tous
  Init_mutex_and_check(mutexes_and_cond->Available_telephone);
  Init_mutex_and_check(mutexes_and_cond->Available_cooks);
  Init_mutex_and_check(mutexes_and_cond->Available_ovens);
  Init_mutex_and_check(mutexes_and_cond->Available_deli_guys);
  Init_mutex_and_check(mutexes_and_cond->Available_packet_guy);
  Init_mutex_and_check(mutexes_and_cond->Update_income);
  Init_mutex_and_check(mutexes_and_cond->Update_orders);
  Init_mutex_and_check(mutexes_and_cond->Update_wait_time);
  Init_mutex_and_check(mutexes_and_cond->Writing_in_stdout);
  Init_cond_and_check(mutexes_and_cond->Telephone_cond);
  Init_cond_and_check(mutexes_and_cond->Cooks_cond);
  Init_cond_and_check(mutexes_and_cond->Ovens_cond);
  Init_cond_and_check(mutexes_and_cond->Cooks_cond);
  Init_cond_and_check(mutexes_and_cond->Deli_guys_cond);
  Init_cond_and_check(mutexes_and_cond->Packet_guy_cond);
  
}

void Initialize_global_data(struct Global_data* global_data,Init_file_data file_data){
 
  // global_data->orders_array=malloc(file_data.oven* sizeof(Order));
  // Initialize_orders_array(global_data, &file_data);
  global_data->telephones_available=file_data.n_tel;
  global_data->ovens_available=file_data.oven;
  global_data->total_wait_time=0;
  global_data->average_wait_time=0.0;
  global_data->average_service_time=0.0;
  global_data->maximum_wait_time=0;
  global_data->maximum_service_time=0;  
  global_data->total_failed_transactions=0;
  global_data->total_income=0;
  global_data->cooks_available=file_data.n_cook;
  global_data->packet_guy=1;
  global_data->deliguys_available=file_data.n_deliverer;
  global_data->num_of_fails=0;

}
void printPizza(){
  printf("      _              \n");
  printf("     (_)             \n");
  printf("_ __  _ ____________ \n");
  printf("| '_\\| |_  /_  / _ `|\n");
  printf("| |_)| |/ / / / (_| |\n");
  printf("| ._/|_/___/__\\__,_|\n");
  printf("| |                   \n");
  printf("|_| \n\n");
}
int main(int argc, char** argv){
  //fortwnoume ta program arguments
	Argument_data data = Get_Arguments(argc,argv);
  printf("----Welcome!----\n");
  printf("Authors: chatzo-giannakoulias\n");

  printPizza();
  
	srand(data.rand_seed);
  //fortwnoume apo to txt tis metablites. go to method for more
  Init_file_data file_data = Get_data_from_file();
  //arxikopoioume ta mutexes
	Initialize_mutexes(&mutexes_and_cond);
	Initialize_global_data(&global_data,file_data);
	//MAIN PHASE
  //ftiaxoume ta clients_N threads me vash ta args. 
	pthread_t clients[data.clients_num];
	Start_Clients(clients, data.clients_num, file_data);
  //elegxei gia sfalmata
	Wait_for_clients_to_finish(clients, data.clients_num);
	//FINISHING 
  //stats:
  printf("\n ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \n");
	printf("\n~~~~Today's clients:   %d \n", data.clients_num);
  printf("----Total failed transactions = %d \n",global_data.num_of_fails);
	Print_Stats(data, file_data);
  printf("\n p3150024, p31500197 kai kala mas ptyxia!\n");
	//memory cleanup
  Destroy_Mutexes_and_cond();
}
