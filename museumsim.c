#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#include "museumsim.h"

//
// In all of the definitions below, some code has been provided as an example
// to get you started, but you do not have to use it. You may change anything
// in this file except the function signatures.
//


struct shared_data {
	// Add any relevant synchronization constructs and shared state here.
	// For example:
	//     pthread_mutex_t ticket_mutex;
	//     int tickets;

    int visitors_touring;
    int guides_touring;
    int visitors_waiting;
	int guides_waiting;
	int allow_entry;

	int tickets;

    pthread_mutex_t shared_memory_lock;

	//pthread_cond_t entered;

	pthread_cond_t guideOutside;
	pthread_cond_t guideTouring;
	pthread_cond_t visitorOutside;
	pthread_cond_t visitorTouring;

};

static struct shared_data shared;

void cap_visitors(){


}

void allow_visitors_in(){

}

/**
 * Set up the shared variables for your implementation.
 * 
 * `museum_init` will be called before any threads of the simulation
 * are spawned.
 */
void museum_init(int num_guides, int num_visitors)
{
	// pthread_mutex_init(&shared.ticket_mutex, NULL);
	//
	shared.tickets = MIN(VISITORS_PER_GUIDE * num_guides, num_visitors);
	shared.visitors_waiting = 0;
	shared.allow_entry = 0;

	pthread_mutex_init(&shared.shared_memory_lock, NULL);

	pthread_cond_init(&shared.guideOutside, NULL); //Wait2
	pthread_cond_init(&shared.visitorOutside, NULL);//Wait1
	pthread_cond_init(&shared.guideTouring, NULL);//Wait3
	pthread_cond_init(&shared.visitorTouring, NULL);//Wait4
}


/**
 * Tear down the shared variables for your implementation.
 * 
 * `museum_destroy` will be called after all threads of the simulation
 * are done executing.
 */
void museum_destroy()
{
	pthread_mutex_destroy(&shared.shared_memory_lock);

	pthread_cond_destroy(&shared.guideOutside);
	pthread_cond_destroy(&shared.visitorOutside);
	pthread_cond_destroy(&shared.guideTouring);
	pthread_cond_destroy(&shared.visitorTouring);


	/*
	pthread_cond_init(&shared.guideOutside, NULL); //Wait2
	pthread_cond_init(&shared.visitorOutside, NULL);//Wait1
	pthread_cond_init(&shared.guideTouring, NULL);//Wait3
	pthread_cond_init(&shared.visitorTouring, NULL);//Wait4
	*/

}


/**
 * Implements the visitor arrival, touring, and leaving sequence.
 */
void visitor(int id)
{
	// visitor_arrives(id);
	// visitor_tours(id);
	// visitor_leaves(id);

	visitor_arrives(id);
	pthread_mutex_lock(&shared.shared_memory_lock);

		if(shared.tickets == 0){
			visitor_leaves(id);
			pthread_mutex_unlock(&shared.shared_memory_lock);
			return;
			}	//Go home if no tickets
		
		shared.visitors_waiting += 1;	//Wait outside
		shared.tickets -= 1;			//One ticket from the maximum


		//Signal Wait3
		pthread_cond_broadcast(&shared.guideTouring);
		//Visitor waits until it gets admitted by a guide.
		while(shared.allow_entry == 0){	//No guide touring. Wait for one to start a tour. How to cap this at 10?
			pthread_cond_wait(&shared.visitorOutside, &shared.shared_memory_lock);
		}
			//Wait 1

		//Visitor Tours
		shared.allow_entry -= 1;
		pthread_mutex_unlock(&shared.shared_memory_lock);

		visitor_tours(id); //released and start to tour

		pthread_mutex_lock(&shared.shared_memory_lock);

		visitor_leaves(id);

		shared.visitors_touring -=1;

		pthread_cond_broadcast(&shared.visitorTouring);
		pthread_mutex_unlock(&shared.shared_memory_lock);
		//Visitor Leaves
			//Signal Wait4

}

/**
 * Implements the guide arrival, entering, admitting, and leaving sequence.
 */
void guide(int id)
{
	// guide_arrives(id);
	// guide_enters(id);
	// guide_admits(id);
	// guide_leaves(id);


	pthread_mutex_lock(&shared.shared_memory_lock);
	//Guide arrives
	guide_arrives(id);

	
	//Guide Waits if guides touring is greater than or equal to two.
	while(shared.guides_touring >= 2){
		pthread_cond_wait(&shared.guideOutside, &shared.shared_memory_lock);//Wait 2
	}

	guide_enters(id);


	//While visitors_served < 10 && there are still visitors to serve
	//pthread_mutex_lock(&shared.shared_memory_lock);
	shared.guides_touring +=1;
	//pthread_mutex_unlock(&shared.shared_memory_lock);
	int served = 0;
	while(served < 10 && (shared.tickets > 0 || shared.visitors_waiting > 0)){
		//Guide Waits until one visitor arrives
		while(shared.visitors_waiting == 0 && shared.tickets > 0){
			pthread_cond_wait(&shared.guideTouring, &shared.shared_memory_lock);
		}

			//Wait 3

		guide_admits(id);//Guide admits 1 visitor
		shared.visitors_waiting -=1;
		shared.visitors_touring +=1;
		served +=1;
		shared.allow_entry += 1;
		pthread_cond_signal(&shared.visitorOutside);
			//Signal-Wait1
		
	}

	//if(served >= 10 || served < 10 && shared.tickets == 0)


	//Guide waits for visitors to leave
	while(shared.visitors_touring != 0){
		pthread_cond_wait(&shared.visitorTouring, &shared.shared_memory_lock);
	}

		//Wait4

	guide_leaves(id);//Guide leaves
	shared.guides_touring -= 1;
	if(shared.guides_touring == 0){
		pthread_cond_broadcast(&shared.guideOutside);//Signal-Wait2
	}


	pthread_mutex_unlock(&shared.shared_memory_lock);



}
