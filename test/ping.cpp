#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <raikv/shm_ht.h>
#include <raikv/key_buf.h>

using namespace rai;
using namespace kv;

HashTabGeom geom;
HashTab   * map;
uint32_t    ctx_id = MAX_CTX_ID;
extern void print_map_geom( HashTab *map,  uint32_t ctx_id );

static void
shm_attach( const char *mn )
{
  map = HashTab::attach_map( mn, 0, geom );
  if ( map != NULL ) {
    ctx_id = map->attach_ctx( ::getpid() );
    print_map_geom( map, ctx_id );
  }
}

static void
shm_close( void )
{
  if ( ctx_id != MAX_CTX_ID ) {
    HashCounters & stat = map->ctx[ ctx_id ].stat;
    printf( "rd %" PRIu64 ", wr %" PRIu64 ", "
            "sp %" PRIu64 ", ch %" PRIu64 "\n",
            stat.rd, stat.wr, stat.spins, stat.chains );
    map->detach_ctx( ctx_id );
    ctx_id = MAX_CTX_ID;
  }
  delete map;
}

#if 0
static void
my_yield( void )
{
  pthread_yield();
}
#endif

void
do_spin( uint64_t cnt )
{
  __asm__ __volatile__(
       "xorq %%rcx, %%rcx\n\t"
    "1: addq $1, %%rcx\n\t"
       "cmpq %0, %%rcx\n\t"
       "jne 1b\n\t"
    :
    : "r" (cnt)
    : "%rcx" );

}

struct PingRec {
  uint64_t ping_time,
           pong_time,
           serial;
};

int
main( int argc, char *argv[] )
{
  SignalHandler sighndl;
  KeyBuf        pingkb, pongkb;
  KeyCtxAlloc8k wrk;
  uint64_t      size;
  uint64_t      last_serial = 0, mbar = 0, spin_times;
  uint64_t      last_count = 0, count = 0, nsdiff = 0, diff, t, last = 0;
  bool          use_pause, use_none, use_spin;
  const char  * pausesp = NULL,
              * oper    = NULL,
              * mn      = NULL;

  pingkb.set_string( "ping" );
  pongkb.set_string( "pong" );
  /*if ( argc > 1 && ::strcmp( argv[ 1 ], "ping" ) == 0 )
    shm_open();
  else*/
  spin_times = 200;
#if 0
  uint64_t t2;
  do {
    t = current_monotonic_time_ns();
    spin_times++;
    do_spin( spin_times * 100000 );
    t2 = current_monotonic_time_ns();
  } while ( t2 - t < 15 * 100000 );
  printf( "do_spin ( %" PRIu64 " ) = 15ns\n", spin_times );
#endif
  if ( argc <= 2 ) {
  cmd_error:;
    fprintf( stderr,
     "%s (map) (ping|pong) (pause|none|spin)\n"
     "  map             -- name of map file (prefix w/ file:, sysv:, posix:)\n"
     "  ping|pong       -- which queue to read\n"
     "  pause|spin|none -- method of read spin loop\n",
             argv[ 0 ]);
    return 1;
  }
  switch ( argc ) {
    default: goto cmd_error;
    case 4: pausesp  = argv[ 3 ];
    case 3: oper     = argv[ 2 ];
            mn       = argv[ 1 ];
            break;
  }

  shm_attach( mn );
  if ( map == NULL )
    return 1;
  sighndl.install();

  KeyCtx pingctx( *map, ctx_id, &pingkb ),
         pongctx( *map, ctx_id, &pongkb );
  uint64_t h1, h2;
  h1 = map->hdr.hash_key_seed;
  h2 = map->hdr.hash_key_seed2;
  pingkb.hash( h1, h2 );
  pingctx.set_hash( h1, h2 );
  h1 = map->hdr.hash_key_seed;
  h2 = map->hdr.hash_key_seed2;
  pongkb.hash( h1, h2 );
  pongctx.set_hash( h1, h2 );

  if ( pausesp != NULL && ::strcmp( pausesp, "pause" ) == 0 )
    use_pause = true;
  else
    use_pause = false;
  if ( pausesp != NULL && ::strcmp( pausesp, "none" ) == 0 )
    use_none = true;
  else
    use_none = false;
  use_spin = ( ! use_pause && ! use_none );
  printf( "use_pause %s\n", use_pause ? "true" : "false" );
  printf( "use_spin  %s\n", use_spin ? "true" : "false" );
  printf( "use_none  %s\n", use_none ? "true" : "false" );
  fflush( stdout );

  if ( ::strcmp( oper, "ping" ) == 0 ) {
    PingRec init, *ptr;
    ::memset( &init, 0, sizeof( init ) );
    //init.ping_time = current_monotonic_time_ns();
    init.ping_time = get_rdtsc();
    init.serial    = 1;
    if ( pingctx.acquire( &wrk ) <= KEY_IS_NEW ) {
      if ( pingctx.resize( &ptr, sizeof( PingRec ) ) == KEY_OK )
        *ptr = init;
      pingctx.release();
    }
    if ( pongctx.acquire( &wrk ) <= KEY_IS_NEW ) {
      if ( pongctx.value( &ptr, size ) == KEY_OK ) {
        init = *ptr;
        init.serial++;
        if ( pongctx.resize( &ptr, sizeof( PingRec ) ) == KEY_OK )
          *ptr = init;
      }
      pongctx.release();
    }
    last_serial = 1;
    while ( ! sighndl.signaled ) {
      if ( pingctx.find( &wrk ) == KEY_OK &&
           pingctx.value( &ptr, size ) == KEY_OK ) {
        if ( size >= sizeof( PingRec ) &&
             ptr->serial > last_serial ) {
          uint64_t pong_time = ptr->pong_time;
          last_serial = ptr->serial;
          t = get_rdtsc();
          if ( t > ptr->ping_time ) {
            diff = t - ptr->ping_time;
            nsdiff += diff;
            count++;
          }
          KeyStatus status;
          if ( (status = pongctx.acquire( &wrk )) <= KEY_IS_NEW ) {
            if ( (status = pongctx.value( &ptr, size )) == KEY_OK ) {
              init = *ptr;
              init.serial++;
              init.ping_time = t;
              init.pong_time = pong_time;
              if ( pongctx.resize( &ptr, sizeof( PingRec ) ) == KEY_OK ) {
                *ptr = init;
              }
            }
            else {
              printf( "status %d\n", status );
            }
            pongctx.release();
          }
          else {
            printf( "acquire status %d\n", status );
          }
          if ( t - last >= (uint64_t) 2000000000U ) {
            double mb = (double) mbar / (double) ( count - last_count );
            printf( "ping %.1fcy mb=%.1f %" PRIu64 "\n",
                    (double) ( t - last ) / (double) ( count - last_count ),
                    mb, spin_times );
            last_count = count;
            last = t;
            if ( use_spin ) {
              if ( mb > 0.6 )
                spin_times++;
              else if ( mb <= 0.6 && spin_times > 1 )
                spin_times--;
            }
            mbar = 0;
          }
          goto got_ping;
        }
      }
      mbar++;
      //memory_barrier();
    got_ping:;
      //usleep( 100 );
      if ( use_pause )
        kv_sync_pause();
      else if ( use_spin )
        do_spin( spin_times );
    }
  }
  else {
    PingRec init, *ptr;
    ::memset( &init, 0, sizeof( init ) );
    //init.ping_time = current_monotonic_time_ns();
    init.ping_time = get_rdtsc();
    init.serial    = 1;
    if ( pongctx.acquire( &wrk ) <= KEY_IS_NEW ) {
      if ( pongctx.resize( &ptr, sizeof( PingRec ) ) == KEY_OK )
        *ptr = init;
      pongctx.release();
    }
    last_serial = 1;
    while ( ! sighndl.signaled ) {
      if ( pongctx.find( &wrk ) == KEY_OK &&
           pongctx.value( &ptr, size ) == KEY_OK ) {
        if ( size >= sizeof( PingRec ) &&
             ptr->serial > last_serial ) {
          uint64_t ping_time = ptr->ping_time;
          last_serial = ptr->serial;
          init = *ptr;
          t = get_rdtsc();
          if ( t > ptr->pong_time ) {
            diff = t - ptr->pong_time;
            nsdiff += diff;
            count++;
          }
          KeyStatus status;
          if ( (status = pingctx.acquire( &wrk )) <= KEY_IS_NEW ) {
            if ( (status = pingctx.value( &ptr, size )) == KEY_OK ) {
              init = *ptr;
              init.serial++;
              init.pong_time = t;
              init.ping_time = ping_time;
              if ( pingctx.resize( &ptr, sizeof( PingRec ) ) == KEY_OK ) {
                *ptr = init;
              }
            }
            else {
              printf( "status %d\n", status );
            }
            pingctx.release();
          }
          else {
            printf( "acquire status %d\n", status );
          }
          if ( t - last >= (uint64_t) 2000000000U ) {
            double mb = (double) mbar / (double) ( count - last_count );
            printf( "pong %.1fcy mb=%.1f %" PRIu64 "\n",
                    (double) ( t - last ) / (double) ( count - last_count ),
                    mb, spin_times );
            last_count = count;
            last = t;
            if ( use_spin ) {
              if ( mb > 0.6 )
                spin_times++;
              else if ( mb <= 0.6 && spin_times > 1 )
                spin_times--;
            }
            mbar = 0;
          }
          goto got_pong;
        }
      }
      mbar++;
      //memory_barrier();
    got_pong:;
      //usleep( 100 );
      if ( use_pause )
        kv_sync_pause();
      else if ( use_spin )
        do_spin( spin_times );
    }
  }
  shm_close();
  printf( "count %" PRIu64 ", diff %.1f\n", count,
          (double) nsdiff / (double) count );
  return 0;
}

