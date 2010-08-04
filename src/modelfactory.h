/***************************************************************************
 *   Copyright (C) 2009 by BUI Quang Minh   *
 *   minh.bui@univie.ac.at   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef MODELFACTORY_H
#define MODELFACTORY_H

#include "tools.h"
#include "substmodel.h"

/**
Store the transition matrix corresponding to evolutionary time so that one must not compute again. 
For efficiency purpose esp. for protein (20x20) or codon (61x61).
The values of the map contain 3 matricies consecutively: transition matrix, 1st, and 2nd derivative

	@author BUI Quang Minh <minh.bui@univie.ac.at>
*/
class ModelFactory : public hash_map<int, double*>
{
public:

	/**
		constructor
		@param amodel pointer for the actual model
	*/
	ModelFactory(SubstModel *amodel, bool store_matrix);

	void startStoringTransMatrix();
	void stopStoringTransMatrix();

	/**
		Wrapper for computing the transition probability matrix from the model. It use ModelFactory
		that stores matrix computed before for effiency purpose.
		@param time time between two events
		@param trans_matrix (OUT) the transition matrix between all pairs of states. 
			Assume trans_matrix has size of num_states * num_states.
	*/
	void computeTransMatrix(double time, double *trans_matrix);

	/**
		Wrapper for computing the transition probability matrix and the derivative 1 and 2 from the model.
		It use ModelFactory that stores matrix computed before for effiency purpose.
		@param time time between two events
		@param trans_matrix (OUT) the transition matrix between all pairs of states. 
			Assume trans_matrix has size of num_states * num_states.
		@param trans_derv1 (OUT) the 1st derivative matrix between all pairs of states. 
		@param trans_derv2 (OUT) the 2nd derivative matrix between all pairs of states. 
	*/
	void computeTransDerv(double time, double *trans_matrix, 
		double *trans_derv1, double *trans_derv2);

	/**
		 destructor
	*/
    ~ModelFactory();

	/**
		pointer to the model, will not be deleted when deleting ModelFactory object
	*/
	SubstModel *model;

	/**
		TRUE to store transition matrix into this hash table for computation efficiency
	*/
	bool store_trans_matrix;

	/**
		TRUE for storing process
	*/
	bool is_storing;
};

#endif
