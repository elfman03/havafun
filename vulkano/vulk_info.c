/*
 * Vulkano HavaFun code component
 * Code to dump some diagnostic responses from Vulkano
 *
 * Copyright (C) 2009-2013 Chris Elford
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option) 
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
 * more details.
 *
 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, see <http://www.gnu.org/licenses>.
 *
 * Linking havafun/vulkfun statically or dynamically with other modules is 
 * making a combined work based on havafun/vulkfun. Thus, the terms and 
 * conditions of the GNU General Public License cover the whole combination.
 *
 * In addition, as a special exception, the copyright holders of havafun/vulkfun
 * give you permission to combine havafun with free software programs or 
 * libraries that are released under the GNU LGPL and with code included in the 
 * standard release of "myhava.com Hava Player (or myvulkano.com Vulkano Player)
 * platform SDKs (e.g., linux, mac, windows)" under "myhava.com or 
 * myvulkano.com's SDK license" (or modified versions of such code, with 
 * unchanged license). You may copy and distribute such a system following the 
 * terms of the GNU GPL for havafun/vulkfun and the licenses of the other code 
 * concerned {, provided that you include the source code of that other code 
 * when and as the GNU GPL requires distribution of source code}.
 *
 * Note that people who make modified versions of havafun/vulkfun are not 
 * obligated to grant this special exception for their modified versions; it 
 * is their choice whether to do so. The GNU General Public License gives 
 * permission to release a modified version without this exception; this 
 * exception also makes it possible to release a modified version which 
 * carries forward this exception.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "vulk_util.h"

Usage() {
  fprintf(stderr,"Usage:  vulk_info\n");
  fprintf(stderr,"        NOTE: Exit from all Vulkano client programs before running vulk_info.\n\n");
#ifdef VSTUDIO
  fprintf(stderr,"Windows NOTE: Also run \"net stop havasvc\" before running vulk_info.\n");
  Vulk_finishup();
#endif
  exit(1);
}

main(int argc, char *argv[]) 
{
  Vulk *vulk;

  Vulk_startup(stderr);

  if(argc!=1) { Usage(); }

  vulk=Vulk_alloc("-",1,0,stderr,1);
  if(!Vulk_isbound(vulk)) { 
    fprintf(stderr,"Error: Could not bind to port 1778 in order to recv()\n");
    Usage() ; 
  }

  printf("Watching for info\n");

  Vulk_sendcmd(vulk, HAVA_INIT, 0, 0); 
  Vulk_loop(vulk,HAVA_MAGIC_INIT,1); 

  Vulk_close(vulk);
  Vulk_finishup();
}
