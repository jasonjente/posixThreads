#ifndef PIZZA_H
#define PIZZA_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


struct Order{
	int order_id;
} typedef Order;

struct Mutexes_and_cond{
	//me auta paizoume mpala sta mutex. ta lockaroume analoga ti peristash
	//kai otan teleiwsoume opoia diadikasia kanoume kanoume unlock
	pthread_mutex_t Available_telephone;
	pthread_mutex_t Available_cooks;
	pthread_mutex_t Available_ovens;
	pthread_mutex_t Available_deli_guys;
	pthread_mutex_t Available_packet_guy;
	
	pthread_mutex_t Update_income;
	pthread_mutex_t Update_transaction_counter;
	pthread_mutex_t Update_wait_time;
	pthread_mutex_t Update_orders;
	pthread_mutex_t Writing_in_stdout; //screenlock
	//ta conditions pou xrisimopoioume
	//tel gia tous tilefonites, oso den uparxei diathesimos to thread perimenei
	pthread_cond_t Telephone_cond;
	//omoiws gia tous mageires, tha perimenei na uparxei kapoios mageiras gia 
	//na analavei tin proetoimasia
	pthread_cond_t Cooks_cond;
	//auto to cond to xrisimopoioume gia na perimenoume fournous, toulaxiston 
	//ena fourno gia kathe pitsa apo ti paraggelia, an px exoume 5 pizzes paraggelia
	// kai tesseris fournous, tha perimenei na eleutherwthei enas gia na piasei
	// kai tous 5 fournous
	pthread_cond_t Ovens_cond;
	//omoiws me to tilefoniti, analambanei mia paraggelia efoson einai diathesimos
	pthread_cond_t Deli_guys_cond;
	pthread_cond_t Packet_guy_cond;

}typedef Mutexes_and_cond;

struct Global_data{
	//sunolikos xronos anamonis
	int total_wait_time;	
	//sunolikos xronos treksimatos nimatos
	int total_throughput_time;
	//mesos xronos anamonis 
	double average_wait_time;
	//megistos xronos anamonis pizzas apo tin arxi tis paraggelias
	int maximum_wait_time;
	//mesos xronos apo tin stigmi p vgikan oi pizzes apo to fourno
	double average_service_time;
	//max xronos apo tin stigmi p vgikan oi pizzes apo to fourno
	int maximum_service_time;
	int total_failed_transactions;
	int total_transactions;
	int total_income;
	int telephones_available;
	int ovens_available;
	int cooks_available;
	int packet_guy;
	int deliguys_available;
	int num_of_fails;
	Order* orders_array;
	
}typedef Global_data;

struct Argument_data{
	//prorgram arguments
	const int clients_num;
	const int rand_seed;

}typedef Argument_data;


struct Const_times{
	 int t_prep;
	 int t_bake;
	 int t_del_low;
	 int t_del_high;
	 int t_pack;

}typedef Const_times;

struct Init_file_data{
	//data apo to txt arxeio
	int n_tel;
	int n_cook;
	int oven;
	int n_deliverer;
	int t_order_low;
	int t_order_high;
	int n_order_low;
	int n_order_high;
	int t_payment_low;
	int t_payment_high;
	int c_pizza;
	double p_fail;
	int t_bake;
	int t_prep;
	int t_pack;
	int del_low;
	int del_high;

}typedef Init_file_data;

struct thread_arg{
	int t_order_low;
	int t_order_high;
	int n_order_low;
	int n_order_high;
	int t_payment_low;
	int t_payment_high;
	int t_prep;
	int t_bake;
	int t_pack;
	double p_fail;
	int c_pizza;
	int order_id;
	struct Const_times const_arg;
}typedef thread_arg;

struct Server_return_data{
	int pizzas;
	int transaction_id;
	int transaction_cost;
	int error_num;
	int flag;
	int wait_time;
}typedef Server_return_data;

Mutexes_and_cond mutexes_and_cond;
Global_data global_data;

#endif
