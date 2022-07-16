//	Copyright (c) 1988 AT&T
//	All Rights Reserved 
//	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
//	The copyright notice above does not evidence any 
//	actual or intended publication of such source code.

#ident	"@(#)sdb:libdbgen/common/Avltree.C	1.3"
#include	"Avltree.h"
#include	<stdio.h>

typedef enum	{ LEFT_HEAVY, BALANCED, RIGHT_HEAVY, LL_NEEDED, LR_NEEDED,
			RL_NEEDED, RR_NEEDED };

typedef enum	{ LEFT, FOUND, RIGHT, NOTREE };

char *bal[] = {	"LEFT HEAVY", "BALANCED", "RIGHT HEAVY", "LL_NEEDED",
			"LR_NEEDED", "RL_NEEDED", "RR_NEEDED" };

static char *dir[] = {	"LEFT", "FOUND", "RIGHT", "NOTREE" };

struct pathnode {
	Avlnode *	node;
	int		direction;
};

Avlnode::Avlnode()
{
	leftchild = rightchild = 0;
	prev = succ = 0;
	balance = BALANCED;
	height = 1;
}

Avlnode *
Avlnode::makenode()
{
	return new Avlnode;
}

int
Avlnode::operator>( Avlnode & )
{
	return 1;
}

int
Avlnode::operator<( Avlnode & )
{
	return 1;
}

void
Avlnode::value_swap( Avlnode * )
{
}

void
Avlnode::reset()
{
	short	lht,rht;

	lht = (leftchild == 0)? 0 : leftchild->height;
	rht = (rightchild == 0)? 0 : rightchild->height;
	if ( (lht > (rht+1)) && leftchild->balance == LEFT_HEAVY )
	{
		balance = LL_NEEDED;
		height = lht + 1;
	}
	else if ( (rht > (lht+1)) && rightchild->balance == RIGHT_HEAVY )
	{
		balance = RR_NEEDED;
		height = rht + 1;
	}
	else if (lht > (rht+1))
	{
		balance = LR_NEEDED;
		height = lht + 1;
	}
	else if (rht > (lht+1))
	{
		balance = RL_NEEDED;
		height = rht + 1;
	}
	else if ( lht < rht )
	{
		balance = RIGHT_HEAVY;
		height = rht + 1;
	}
	else if ( lht == rht )
	{
		balance = BALANCED;
		height = lht + 1;
	}
	else
	{
		balance = LEFT_HEAVY;
		height = lht + 1;
	}
}

Avlnode *
Avlnode::next()
{
	return succ;
}

Avlnode *
Avlnode::last()
{
	return prev;
}

Avltree::Avltree()
{
	root = 0;
	leftmost_link = rightmost_link = 0;
	search_path = new pathnode[30];
}

void
Avltree::adjust_up( Avlnode * node )
{
	Avlnode *		xnode;
	Avlnode *		lower_node;
	Avlnode *		child;
	struct pathnode *	x;

	child = node;
	x = p;
	lower_node = p->node;
	while ( x->direction != NOTREE )
	{
		xnode = x->node;
		if ( x->direction == LEFT )
		{
			xnode->leftchild = child;
		}
		else if ( x->direction == RIGHT )
		{
			xnode->rightchild = child;
		}
		xnode->reset();
		if ( xnode->balance == LL_NEEDED )
		{
			lower_node = ll_rebalance( xnode );
		}
		else if ( xnode->balance == LR_NEEDED )
		{
			lower_node = lr_rebalance( xnode );
		}
		else if ( xnode->balance == RL_NEEDED )
		{
			lower_node = rl_rebalance( xnode );
		}
		else if ( xnode->balance == RR_NEEDED )
		{
			lower_node = rr_rebalance( xnode );
		}
		else
		{
			lower_node = xnode;
		}
		child = lower_node;
		if ( xnode == root )
		{
			root = child;
		}
		--x;
	}
}

Avlnode *
Avltree::locate( Avlnode & node, Avlnode * subtree )
{
	Avlnode *	x;

	x = subtree;
	while ( x != 0 )
	{
		++p;
		p->node = x;
		if ( *x < node )
		{
			p->direction = RIGHT;
			lastright = x;
			x = x->rightchild;
		}
		else if ( *x > node )
		{
			p->direction = LEFT;
			lastleft = x;
			x = x->leftchild;
		}
		else
		{
			p->direction = FOUND;
			break;
		}
	}
	return x;
}

Avlnode *
Avltree::tinsert( Avlnode & node )
{
	Avlnode *	subtree;
	Avlnode *	leaf;

	lastleft = lastright = 0;
	p = search_path;
	p->direction = NOTREE;
	subtree = locate ( node, root );
	if ( p->direction == NOTREE )
	{
		leaf = node.makenode();
		root = leaf;
		leftmost_link = leaf;
		rightmost_link = leaf;
	}
	else if ( p->direction == FOUND )
	{
		leaf = 0;
	}
	else
	{
		leaf = node.makenode();
		leaf->prev = lastright;
		leaf->succ = lastleft;
		if ( lastright == 0 )
			leftmost_link = leaf;
		else
			lastright->succ = leaf;
		if ( lastleft == 0 )
			rightmost_link = leaf;
		else
			lastleft->prev = leaf;
		adjust_up( leaf );
	}
	return leaf;
}

Avlnode *
Avltree::tlookup( Avlnode & node )
{
	lastleft = lastright = 0;
	p = search_path;
	p->direction = NOTREE;
	return locate( node, root );
}

Avlnode *
Avltree::rr_rebalance( Avlnode * critical )
{
	Avlnode *	newroot;

	newroot = critical->rightchild;
	critical->rightchild = newroot->leftchild;
	newroot->leftchild = critical;
	critical->reset();
	newroot->reset();
	return newroot;
}

Avlnode *
Avltree::rl_rebalance( Avlnode * critical )
{
	Avlnode *	newroot;
	Avlnode *	rchild;

	newroot = critical->rightchild->leftchild;
	rchild = critical->rightchild;
	critical->rightchild = newroot->leftchild;
	rchild->leftchild = newroot->rightchild;
	newroot->leftchild = critical;
	newroot->rightchild = rchild;
	critical->reset();
	rchild->reset();
	newroot->reset();
	return newroot;
}

Avlnode *
Avltree::lr_rebalance( Avlnode * critical )
{
	Avlnode *	newroot;
	Avlnode *	lchild;

	newroot = critical->leftchild->rightchild;
	lchild = critical->leftchild;
	critical->leftchild = newroot->rightchild;
	lchild->rightchild = newroot->leftchild;
	newroot->rightchild = critical;
	newroot->leftchild = lchild;
	critical->reset();
	lchild->reset();
	newroot->reset();
	return newroot;
}

Avlnode *
Avltree::ll_rebalance( Avlnode * critical )
{
	Avlnode *	newroot;

	newroot = critical->leftchild;
	critical->leftchild = critical->leftchild->rightchild;
	newroot->rightchild = critical;
	critical->reset();
	newroot->reset();
	return newroot;
}

Avlnode *
Avltree::tfirst()
{
	return leftmost_link;
}

Avlnode *
Avltree::tlast()
{
	return rightmost_link;
}

int
Avltree::tdelete( Avlnode & node )
{
	Avlnode *	d_n;
	Avlnode *	x;
	Avlnode *	y;
	Avlnode *	pnode;
	Avlnode *	leaf;

	lastleft = lastright = 0;
	p = search_path;
	p->direction = NOTREE;
	d_n = locate( node, root );
	if ( p->direction == FOUND && d_n->leftchild == 0 && d_n->rightchild == 0 )
	{
		if ( d_n->prev != 0 )
			d_n->prev->succ = d_n->succ;
		else
			leftmost_link = d_n->succ;
		if ( d_n->succ != 0 )
			d_n->succ->prev = d_n->prev;
		else
			rightmost_link = d_n->prev;
		if ( d_n == root )
			root = 0;
		delete d_n;
		--p;
		adjust_up( 0 );
		return 1;
	}
	else if ( p->direction == FOUND )
	{
		x = d_n->prev;
		if ( (x == 0) || (x->leftchild != 0) || (x->rightchild != 0) )
			x = d_n->succ;
		--p;
		y = locate ( *x, d_n );
		--p;
		pnode = p->node;
		if ( d_n->prev != 0 && y->leftchild != 0 )
		{
			y->value_swap(y->leftchild);
			leaf = y->leftchild;
			y->leftchild = 0;
		}
		else if ( d_n->succ != 0 && y->rightchild != 0 )
		{
			y->value_swap(y->rightchild);
			leaf = y->rightchild;
			y->rightchild = 0;
		}
		else
		{
			leaf = y;
		}
		d_n->value_swap(leaf);
		if ( leaf->prev != 0 )
			leaf->prev->succ = leaf->succ;
		else
			leftmost_link = leaf->succ;
		if ( leaf->succ != 0 )
			leaf->succ->prev = leaf->prev;
		else
			rightmost_link = leaf->prev;
		delete leaf;
		adjust_up( 0 );
		return 1;
	}
	else
	{
		return 0;
	}
}
