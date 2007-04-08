/*
**	A clone of a famous game.
*/
/**@name ccl.h		-	The clone configuration language headerfile. */
/*
**	(c) Copyright 1998,1999 by Lutz Sammer
**
**	$Id$
*/

#ifndef __CCL_H__
#define __CCL_H__

//@{

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#ifdef USE_CCL
#include <guile/gh.h>			// I use guile for a quick hack
#endif

#ifdef USE_CCL2
#include <string.h>
#include "siod.h"			// now I try SIOD
#endif

/*----------------------------------------------------------------------------
--	Macros
----------------------------------------------------------------------------*/

#ifdef USE_CCL2

#define SCM LISP
#define SCM_UNSPECIFIED NIL
#define gh_null_p(lisp) NULLP(lisp)

#define gh_eq_p(lisp1,lisp2)	EQ(lisp1,lisp2)

#define gh_list_p(lisp)		CONSP(lisp)
#define gh_car(lisp)		car(lisp)
#define gh_cdr(lisp)		cdr(lisp)
#define gh_length(lisp)		nlength(lisp)

#define gh_scm2int(lisp)	(long)FLONM(lisp)
#define gh_int2scm(num)		flocons(num)

#define gh_string_p(lisp)	TYPEP(lisp,tc_string)
#define	gh_scm2newstr(lisp,str) strdup(get_c_string(lisp))
#define	gh_str02scm(str) strcons(strlen(str),str)

#define	gh_vector_p(lisp)	\
	(TYPE(lisp)>=tc_string && TYPE(lisp)<=tc_byte_array)
#define	gh_vector_length(lisp)	nlength(lisp)
#define	gh_vector_ref(lisp,n)	aref1(lisp,n)

#define gh_boolean_p(lisp)	(EQ(lisp,sym_t) || NULLP(lisp))
#define gh_scm2bool(lisp)	(NNULLP(lisp))

#define gh_symbol_p(lisp)	SYMBOLP(lisp)
#define gh_symbol2scm(str)	cintern(str)

#define gh_display(lisp)	lprin1f(lisp,stdout)
#define gh_newline()		fprintf(stdout,"\n")

#define gh_eval_file(str)	vload(str,0,0)

#define gh_new_procedure0_0	init_subr_0
#define gh_new_procedure1_0	init_subr_1
#define gh_new_procedure2_0	init_subr_2
#define gh_new_procedure3_0	init_subr_3
#define gh_new_procedure4_0	init_subr_4
#define gh_new_procedure5_0	init_subr_5
#define gh_new_procedureN	init_lsubr

#define SCM_BOOL_T	sym_t
#define SCM_BOOL_F	NIL

extern LISP sym_t;

#endif	// USE_CCL2

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

extern char* CclStartFile;		/// CCL start file
extern int CclInConfigFile;		/// True while config file parsing

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

extern void CclInit(void);		/// Initialise ccl
extern void CclCommand(char*);		/// Execute a ccl command
extern void CclFree(void*);		/// Save free

#ifdef USE_CCL
extern SCM gh_new_procedureN(char* proc_name, SCM (*fn) ());

#ifdef LIBGUILE12			// my snapshot version 19980430
					// didn't need it
#define gh_vector_ref(vector,index) gh_vref(vector,index)

#endif	// LIBGUILE12

#endif	// USE_CCL

//@}

#endif	// !__CCL_H__