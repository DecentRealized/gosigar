/*
 * Copyright (c) 2000 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.1 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
 * http://www.apple.com/publicsource and read it before using this file.
 * 
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 * Copyright (c) 1988, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)radix.h	8.2 (Berkeley) 10/31/94
 * $FreeBSD: src/sys/net/radix.h,v 1.16.2.1 2000/05/03 19:17:11 wollman Exp $
 */

#ifndef _RADIX_H_
#define	_RADIX_H_
#include <sys/appleapiopts.h>

#if !defined(KERNEL) || defined(__APPLE_API_PRIVATE)

#ifdef MALLOC_DECLARE
MALLOC_DECLARE(M_RTABLE);
#endif

/*
 * Radix search tree node layout.
 */

struct radix_node {
	struct	radix_mask *rn_mklist;	/* list of masks contained in subtree */
	struct	radix_node *rn_parent;	/* parent */
	short	rn_bit;			/* bit offset; -1-index(netmask) */
	char	rn_bmask;		/* node: mask for bit test*/
	u_char	rn_flags;		/* enumerated next */
#define RNF_NORMAL	1		/* leaf contains normal route */
#define RNF_ROOT	2		/* leaf is root leaf for tree */
#define RNF_ACTIVE	4		/* This node is alive (for rtfree) */
	union {
		struct {			/* leaf only data: */
			caddr_t	rn_Key;		/* object of search */
			caddr_t	rn_Mask;	/* netmask, if present */
			struct	radix_node *rn_Dupedkey;
		} rn_leaf;
		struct {			/* node only data: */
			int	rn_Off;		/* where to start compare */
			struct	radix_node *rn_L;/* progeny */
			struct	radix_node *rn_R;/* progeny */
		} rn_node;
	}		rn_u;
#ifdef RN_DEBUG
	int rn_info;
	struct radix_node *rn_twin;
	struct radix_node *rn_ybro;
#endif
};

#define	rn_dupedkey	rn_u.rn_leaf.rn_Dupedkey
#define	rn_key		rn_u.rn_leaf.rn_Key
#define	rn_mask		rn_u.rn_leaf.rn_Mask
#define	rn_offset	rn_u.rn_node.rn_Off
#define	rn_left		rn_u.rn_node.rn_L
#define	rn_right	rn_u.rn_node.rn_R

#if 0
/* for backward compatibility with previous definitions */
#define rn_p 		rn_parent
#define rn_b 		rn_bit
#define rn_off 		rn_offset
#define rn_l 		rn_left
#define rn_r 		rn_right
#endif
/*
 * Annotations to tree concerning potential routes applying to subtrees.
 */

struct radix_mask {
	short	rm_bit;			/* bit offset; -1-index(netmask) */
	char	rm_unused;		/* cf. rn_bmask */
	u_char	rm_flags;		/* cf. rn_flags */
	struct	radix_mask *rm_mklist;	/* more masks to try */
	union	{
		caddr_t	rmu_mask;		/* the mask */
		struct	radix_node *rmu_leaf;	/* for normal routes */
	}	rm_rmu;
	int	rm_refs;		/* # of references to this struct */
};

#define	rm_mask rm_rmu.rmu_mask
#define	rm_leaf rm_rmu.rmu_leaf		/* extra field would make 32 bytes */


#define MKGet(m) {\
	if (rn_mkfreelist) {\
		m = rn_mkfreelist; \
		rn_mkfreelist = (m)->rm_mklist; \
	} else \
		R_Malloc(m, struct radix_mask *, sizeof (*(m))); }\

#define MKFree(m) { (m)->rm_mklist = rn_mkfreelist; rn_mkfreelist = (m);}

typedef int walktree_f_t __P((struct radix_node *, void *));

struct radix_node_head {
	struct	radix_node *rnh_treetop;
	int	rnh_addrsize;		/* permit, but not require fixed keys */
	int	rnh_pktsize;		/* permit, but not require fixed keys */
	struct	radix_node *(*rnh_addaddr)	/* add based on sockaddr */
		__P((void *v, void *mask,
		     struct radix_node_head *head, struct radix_node nodes[]));
	struct	radix_node *(*rnh_addpkt)	/* add based on packet hdr */
		__P((void *v, void *mask,
		     struct radix_node_head *head, struct radix_node nodes[]));
	struct	radix_node *(*rnh_deladdr)	/* remove based on sockaddr */
		__P((void *v, void *mask, struct radix_node_head *head));
	struct	radix_node *(*rnh_delpkt)	/* remove based on packet hdr */
		__P((void *v, void *mask, struct radix_node_head *head));
	struct	radix_node *(*rnh_matchaddr)	/* locate based on sockaddr */
		__P((void *v, struct radix_node_head *head));
	struct	radix_node *(*rnh_lookup)	/* locate based on sockaddr */
		__P((void *v, void *mask, struct radix_node_head *head));
	struct	radix_node *(*rnh_matchpkt)	/* locate based on packet hdr */
		__P((void *v, struct radix_node_head *head));
	int	(*rnh_walktree)			/* traverse tree */
		__P((struct radix_node_head *head, walktree_f_t *f, void *w));
	int	(*rnh_walktree_from)		/* traverse tree below a */
		__P((struct radix_node_head *head, void *a, void *m,
		     walktree_f_t *f, void *w));
	void	(*rnh_close)	/* do something when the last ref drops */
		__P((struct radix_node *rn, struct radix_node_head *head));
	struct	radix_node rnh_nodes[3];	/* empty tree for common case */
};

#ifndef KERNEL
#define Bcmp(a, b, n) bcmp(((char *)(a)), ((char *)(b)), (n))
#define Bcopy(a, b, n) bcopy(((char *)(a)), ((char *)(b)), (unsigned)(n))
#define Bzero(p, n) bzero((char *)(p), (int)(n));
#define R_Malloc(p, t, n) (p = (t) malloc((unsigned int)(n)))
#define Free(p) free((char *)p);
#else
#define Bcmp(a, b, n) bcmp(((caddr_t)(a)), ((caddr_t)(b)), (unsigned)(n))
#define Bcopy(a, b, n) bcopy(((caddr_t)(a)), ((caddr_t)(b)), (unsigned)(n))
#define Bzero(p, n) bzero((caddr_t)(p), (unsigned)(n));
#define R_Malloc(p, t, n) (p = (t) _MALLOC((unsigned long)(n), M_RTABLE, M_WAITOK))
#define Free(p) FREE((caddr_t)p, M_RTABLE);
#endif /*KERNEL*/

void	 rn_init __P((void));
int	 rn_inithead __P((void **, int));
int	 rn_refines __P((void *, void *));
struct radix_node
	 *rn_addmask __P((void *, int, int)),
	 *rn_addroute __P((void *, void *, struct radix_node_head *,
			struct radix_node [2])),
	 *rn_delete __P((void *, void *, struct radix_node_head *)),
	 *rn_lookup __P((void *v_arg, void *m_arg,
		        struct radix_node_head *head)),
	 *rn_match __P((void *, struct radix_node_head *));

#endif /* __APPLE_API_PRIVATE || !KERNEL */
#endif /* _RADIX_H_ */

