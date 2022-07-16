/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:inc/common/Avltree.h	1.1"
#ifndef Avltree_h
#define Avltree_h

class Avlnode {
	Avlnode			*prev, *succ;
	short			height;
	short			balance;
	Avlnode			*leftchild, *rightchild;
	void			reset();
	friend class		Avltree;
public:
				Avlnode();
	virtual Avlnode *	makenode();	// save this in tree memory.
	virtual int		operator<( Avlnode & );
	virtual int		operator>( Avlnode & );
	virtual void		value_swap( Avlnode * );
	Avlnode *		next();
	Avlnode *		last();
};

struct pathnode;

class Avltree {
	Avlnode *		root;
	struct pathnode *	search_path;
	struct pathnode *	p;
	Avlnode *		lastleft;
	Avlnode *		lastright;
	Avlnode *		leftmost_link;
	Avlnode *		rightmost_link;
	Avlnode *		locate( Avlnode &, Avlnode * );
	Avlnode *		rr_rebalance( Avlnode * );
	Avlnode *		rl_rebalance( Avlnode * );
	Avlnode *		lr_rebalance( Avlnode * );
	Avlnode *		ll_rebalance( Avlnode * );
	void			adjust_up( Avlnode * );
public:
				Avltree();
	Avlnode *		tinsert( Avlnode & );
	Avlnode *		tlookup( Avlnode & );
	int			tdelete( Avlnode & );
	int			tdestroy();
	Avlnode *		tfirst();
	Avlnode *		tlast();
};

#endif

// end of Avltree.h

