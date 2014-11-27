/*! \file timer.c
 *
 * \copyright (C) Copyright 2013 University of Antwerp (http://www.cosys-lab.be) and others.\n
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.\n
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * \author jeremie@wizzilab.com
 */
#include "timer.h"

#include "../hal/timer.h"
#include "log.h"

// turn on/off the debug prints
#if 1
#define DPRINT(...) log_print_stack_string(LOG_FWK, __VA_ARGS__)
#else
#define DPRINT(...)  
#endif

#define TIMER_EVENT_STACK_SIZE  20

timer_event timer_event_stack[TIMER_EVENT_STACK_SIZE];
uint8_t     timer_next_event_position;
uint8_t     timer_event_count;
bool        timer_event_running;




void timer_init( void )
{
    hal_timer_init();
    timer_event_count = 0;
    timer_event_running = false;
    uint8_t i;
    for( i=0 ; i<TIMER_EVENT_STACK_SIZE ; i++ )
    {
        timer_event_stack[i].f = NULL;
    }
}


bool timer_add_event( timer_event* event )
{
    timer_event new_event;
    new_event.f = event->f;
    new_event.next_event = event->next_event;
    
    DPRINT("Adding event: t: %d @%p" , new_event.next_event, new_event.f );
    
    // add the new event in the stack
    if( !timer_add_event_in_stack( new_event ) ) { return false; }

    // configure the next event if one is not currently executed
    if( timer_event_running == false )
    {
        // configure the interrupt
        timer_configure_next_event();
    }
    
    return true;
}

void timer_completed( void )
{
    // to avoid configuring annother event when one is still executing
    timer_event_running = true;

    timer_event event;
    uint8_t event_position = timer_next_event_position;
    event.f = timer_event_stack[event_position].f;

    // delete event from stack
    timer_event_stack[event_position].f = NULL;
    
    // update event count
    timer_event_count--;

    // execute event
    event.f();

    DPRINT("Event completed: @%p", event.f);

    // if there is other events waiting
    if( timer_event_count > 0 )
    {
        // prepare the next event
        timer_configure_next_event();
    }
    else
    {
        // disable interrupts to avoid unwanted events
        hal_timer_disable_interrupt();
    }

    // end of running event
    timer_event_running = false;
}

uint32_t timer_get_counter_value( void )
{
    return hal_timer_getvalue();
}

static uint8_t timer_get_next_event( void )
{
    // if the stack is empty
    if( timer_event_count == 0 )
    {
        // trap processor
        log_print_stack_string(LOG_FWK, "TIMER: No events in stack!");
        while(1);
    }

    int32_t next_event_time_temp;
    int32_t next_event_time = 0xFFFFFF; // maximum counter value
    uint8_t next_event_position;
    uint8_t event_count = 0;
    uint8_t i;

    // always update the stack before using it
    timer_update_stack();

    // search for the smaller time in the stack
    for( i=0 ; i<TIMER_EVENT_STACK_SIZE ; i++ )
    {
        if( timer_event_stack[i].f != NULL )
        {
            // return first event found if there is only one event
            if( timer_event_count == 1 )
            {
                DPRINT("One event found: t: %d @%p pos: %d", timer_event_stack[i].next_event, timer_event_stack[i].f, i );
                return i;
            }

            // compare the times to find the smallest and save its position in the stack
            next_event_time_temp = timer_event_stack[i].next_event;

            if( next_event_time_temp <= next_event_time )
            {
                next_event_time = next_event_time_temp;
                next_event_position = i;
            }

            // return the position once all events have been found
            event_count++;
            if( event_count >= timer_event_count )
            {
                DPRINT("Next event found: t: %d @%p pos: %d among %d", timer_event_stack[next_event_position].next_event, timer_event_stack[next_event_position].f, next_event_position, event_count );
                return next_event_position;
            }
        }
    }

    log_print_stack_string(LOG_FWK, "TIMER: error getting next event.");
    while(1);
}


static void timer_configure_next_event( void )
{
    int32_t event_time;

    // retrieve the next event position in the stack
    timer_next_event_position = timer_get_next_event();

    // ajust the value for configuring the interrupt ( register = number of ticks - 1 )
    event_time = timer_event_stack[timer_next_event_position].next_event - 1;

    // if the time is already elapsed
    if( event_time <= hal_timer_getvalue() )
    {
        // fire the event
        DPRINT("Event fired: t: %d @%p pos: %d", event_time, timer_event_stack[event_position].f, event_position);
        timer_completed();
    }
    else
    {
        // set next interrupt
        hal_timer_disable_interrupt();
        hal_timer_setvalue( (uint32_t)event_time );
        hal_timer_enable_interrupt();
        DPRINT("Event configured: t: %d @%p pos: %d", event_time, timer_event_stack[event_position].f, event_position);
    }
}


static bool timer_add_event_in_stack( timer_event new_event )
{
    uint8_t i;
    // add event to the first free spot in the stack
    for( i=0 ; i<TIMER_EVENT_STACK_SIZE ; i++ )
    {
        if( timer_event_stack[i].f == NULL)
        {
            timer_update_stack();
            timer_event_stack[i] = new_event;
            timer_event_count++;

            DPRINT("Event added in stack: t: %d @%p pos: %d", timer_event_stack[i].next_event, timer_event_stack[i].f, i);
            return true;
        }
    }

    log_print_stack_string(LOG_FWK, "TIMER: Stack full!");
    return false;
}

static bool timer_update_stack( void )
{
    // just reset counter if there is no event
    if( timer_event_count == 0 )
    {
        hal_timer_counter_reset();
        return true;
    }

    uint8_t event_count = 0;
    uint8_t i;

    // get the elapsed time
    int32_t current_time = (int32_t)hal_timer_getvalue();

    // reset the counter
    hal_timer_counter_reset();

    for( i=0 ; i<TIMER_EVENT_STACK_SIZE ; i++ )
    {
        if( timer_event_stack[i].f != NULL)
        {
            // substract the elapsed time from the remaining time
            timer_event_stack[i].next_event -= current_time;
            
            // terminate if all events have been updated
            event_count++;
            if( event_count >= timer_event_count )
            {
                DPRINT("Stack updated: time elapsed: %d", current_time);
                return true;
            }
        }
    }

    log_print_stack_string(LOG_FWK, "TIMER: error updating stack.");
    while(1);
}







volatile bool waiting;

void timer_wait_done( void )
{
    waiting = false;
}

timer_event timer_wait = { .next_event = 0, .f = timer_wait_done };

void timer_wait_ms( uint16_t ms )
{
    /*
	waiting = true;
    timer_wait.next_event = ms;
    timer_add_event( &timer_wait );
    while(waiting);
    */
    int i;
    for( i=0 ; i<ms ; i++ )
    {
            volatile uint32_t n = 32000;
            while(n--);
    }
}