/* ****************************************************************** **
**    OpenFRESCO - Open Framework                                     **
**                 for Experimental Setup and Control                 **
**                                                                    **
**                                                                    **
** Copyright (c) 2006, The Regents of the University of California    **
** All Rights Reserved.                                               **
**                                                                    **
** Commercial use of this program without express permission of the   **
** University of California, Berkeley, is strictly prohibited. See    **
** file 'COPYRIGHT_UCB' in main directory for information on usage    **
** and redistribution, and for a DISCLAIMER OF ALL WARRANTIES.        **
**                                                                    **
** Developed by:                                                      **
**   Andreas Schellenberg (andreas.schellenberg@gmx.net)              **
**   Yoshikazu Takahashi (yos@catfish.dpri.kyoto-u.ac.jp)             **
**   Gregory L. Fenves (fenves@berkeley.edu)                          **
**   Stephen A. Mahin (mahin@berkeley.edu)                            **
**                                                                    **
** ****************************************************************** */

// $Revision$
// $Date$
// $Source$

// Written: Andreas Schellenberg (andreas.schellenberg@gmx.net)
// Created: 09/06
// Revision: A
//
// Description: This file contains the implementation of the EEBeamColumn2d class.

#include "EEBeamColumn2d.h"

#include <ArrayOfTaggedObjects.h>
#include <Domain.h>
#include <Node.h>
#include <Channel.h>
#include <FEM_ObjectBroker.h>
#include <Renderer.h>
#include <Information.h>
#include <ElementResponse.h>
#include <CrdTransf2d.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>


// initialize the class wide variables
Matrix EEBeamColumn2d::theMatrix(6,6);
Vector EEBeamColumn2d::theVector(6);
Vector EEBeamColumn2d::theLoad(6);


// responsible for allocating the necessary space needed by each object
// and storing the tags of the ExperimentalBeamColumn end nodes.
EEBeamColumn2d::EEBeamColumn2d(int tag, int Nd1, int Nd2,
    ExperimentalSite *site,
    CrdTransf2d &coordTransf,
    bool iM, bool copy, double r)
    : ExperimentalElement(tag, ELE_TAG_EEBeamColumn2d, site),
    connectedExternalNodes(2), theCoordTransf(0),
    iMod(iM), isCopy(copy), rho(r), L(0.0),
    q(3), targDisp(3), kbInit(3,3), T(3,3), Tinv(3,3)
{
    // ensure the connectedExternalNode ID is of correct size & set values
    if (connectedExternalNodes.Size() != 2)  {
        opserr << "EEBeamColumn2d::EEBeamColumn2d() - element: "
            <<  tag << " failed to create an ID of size 2\n";
        exit(-1);
    }
    
    connectedExternalNodes(0) = Nd1;
    connectedExternalNodes(1) = Nd2;
    
    // set node pointers to NULL
    int i;
    for (i=0; i<2; i++)
        theNodes[i] = 0;
    
    // set the data size for the experimental site
    sizeCtrl = new ID(OF_Resp_All);
    sizeDaq = new ID(OF_Resp_All);
    
    (*sizeCtrl)[OF_Resp_Disp] = 3;
    (*sizeCtrl)[OF_Resp_Vel] = 3;
    (*sizeCtrl)[OF_Resp_Accel] = 3;
    (*sizeCtrl)[OF_Resp_Time] = 1;
    
    (*sizeDaq)[OF_Resp_Disp] = 3;
    (*sizeDaq)[OF_Resp_Force] = 3;
    
    if (isCopy == false)
        theSite->setSize(*sizeCtrl, *sizeDaq);
    
    // set the initial stiffness matrix size
    theInitStif.resize(6,6);
    
    // get a copy of the coordinate transformation
    theCoordTransf = coordTransf.getCopy();
    if (!theCoordTransf)  {
        opserr << "EEBeamColumn2d::EEBeamColumn2d() - "
            << "failed to get copy of the coordinate transformation\n";
        exit(-1);
    }
    
    // initialize vectors
    q.Zero();
    targDisp.Zero();
    for (i=0; i<3; i++)  {
        q0[i] = 0.0;
        p0[i] = 0.0;
    }
    
    // open output file
    //outFile = fopen("elemDisp.out","w");
    //if (outFile==NULL)  {
    //	opserr << "EEBeamColumn2d::EEBeamColumn2d() - "
    //		<< "fopen: could not open output file\n";
    //  exit(-1);
    //}
}


// delete must be invoked on any objects created by the object
// and on the experimental object.
EEBeamColumn2d::~EEBeamColumn2d()
{
    // close output file
    //fclose(outFile);
    
    // invoke the destructor on any objects created by the object
    // that the object still holds a pointer to
    if (theCoordTransf)
        delete theCoordTransf;
}


int EEBeamColumn2d::getNumExternalNodes(void) const
{
    return 2;
}


const ID& EEBeamColumn2d::getExternalNodes(void) 
{
    return connectedExternalNodes;
}


Node** EEBeamColumn2d::getNodePtrs(void) 
{
    return theNodes;
}


int EEBeamColumn2d::getNumDOF(void) 
{
    return 6;
}


int EEBeamColumn2d::getNumBasicDOF(void) 
{
    return 3;
}


// to set a link to the enclosing Domain and to set the node pointers.
// also determines the number of dof associated
// with the ExperimentalBeamColumn element, we set matrix and 
// vector pointers, allocate space for t matrix and define it 
// as the basic deformation-displacement transformation matrix.
void EEBeamColumn2d::setDomain(Domain *theDomain)
{
    // check Domain is not null - invoked when object removed from a domain
    if (!theDomain)  {
        theNodes[0] = 0;
        theNodes[1] = 0;
        
        return;
    }
    
    // first set the node pointers
    theNodes[0] = theDomain->getNode(connectedExternalNodes(0));
    theNodes[1] = theDomain->getNode(connectedExternalNodes(1));	
    
    // if can't find both - send a warning message
    if (!theNodes[0] || !theNodes[1])  {
        if (!theNodes[0])  {
            opserr << "WARNING EEBeamColumn2d::setDomain() - Nd1: " 
                << connectedExternalNodes(0) << " does not exist in the model for ";
        } else  {
            opserr << "WARNING EEBeamColumn2d::setDomain() - Nd2: " 
                << connectedExternalNodes(1) << " does not exist in the model for ";
        }
        opserr << "EEBeamColumn2d ele: " << this->getTag() << endln;
        
        return;
    }
    
    // now determine the number of dof and the dimension    
    int dofNd1 = theNodes[0]->getNumberDOF();
    int dofNd2 = theNodes[1]->getNumberDOF();	
    
    // if differing dof at the ends - print a warning message
    if (dofNd1 != 3)  {
        opserr << "EEBeamColumn2d::setDomain() - node 1: "
            << connectedExternalNodes(0) << " has incorrect number of DOF (not 3)\n";
        return;
    }
    if (dofNd2 != 3)  {
        opserr << "EEBeamColumn2d::setDomain() - node 2: "
            << connectedExternalNodes(1) << " has incorrect number of DOF (not 3)\n";
        return;
    }
    
    // call the base class method
    this->DomainComponent::setDomain(theDomain);
    
    // initialize the coordinate transformation
    if (theCoordTransf->initialize(theNodes[0], theNodes[1]) != 0)  {
        opserr << "EEBeamColumn2d::setDomain() - "
            << "error initializing coordinate transformation\n";
        return;
    }
    
    // get initial element length
    double L = theCoordTransf->getInitialLength();
    if (L == 0.0)  {
        opserr << "EEBeamColumn2d::setDomain() - element: " 
            << this->getTag() << " has zero length\n";
        return;
    }
    
    // set transformation matrix from basic sys A to basic sys B
    T.Zero();
    T(0,0) =  1;
    T(1,1) = -L;
    T(2,1) = -1;  T(2,2) = 1;
    
    // set transformation matrix from basic sys B to basic sys A
    Tinv.Zero();
    Tinv(0,0) =  1;
    Tinv(1,1) = -1/L;
    Tinv(2,1) = -1/L;  T(2,2) = 1;
}   	 


int EEBeamColumn2d::commitState()
{
    int rValue = 0;
    
    if (isCopy == false)
        rValue += theSite->commitState();
    rValue += theCoordTransf->commitState();
    
    return rValue;
}


int EEBeamColumn2d::update(void)
{
    // get current time
    Vector time(1);
    Domain *theDomain = this->getDomain();
    time(0) = theDomain->getCurrentTime();
    
    // update the coordinate transformation
    theCoordTransf->update();
    
    // determine dsp, vel and acc in basic system A
    const Vector &db = theCoordTransf->getBasicTrialDisp();
    const Vector &vb = theCoordTransf->getBasicTrialVel();
    const Vector &ab = theCoordTransf->getBasicTrialAccel();
    
    // transform displacements from basic sys A to basic sys B
    static Vector dbB(3), vbB(3), abB(3);
    dbB = T*db;
    vbB = T*vb;
    abB = T*ab;
    
    if (dbB != targDisp)  {
        // save the target displacement
        targDisp = dbB;
        // set the trial response at the site
        if (isCopy == false)
            theSite->setTrialResponse(&dbB, &vbB, &abB, (Vector*)0, &time);
    }
    
    return 0;
}


int EEBeamColumn2d::setInitialStiff(const Matrix &kbinit)
{
    kbInit = kbinit;
    
    if (kbInit.noRows() != 3 || kbInit.noCols() != 3)  {
        opserr << "EEBeamColumn2d::setInitialStiff() - " 
            << "matrix size is incorrect for element: "
            << this->getTag() << endln;
        return -1;
    }
    
    // transform the stiffness from basic sys B to basic sys A
    static Matrix kbInitA(3,3);
    kbInitA.addMatrixTripleProduct(0.0, T, kbInit, 1.0);
    
    // transform the stiffness from the basic to the global system
    theInitStif.Zero();
    theInitStif = theCoordTransf->getInitialGlobalStiffMatrix(kbInitA);
    
    return 0;
}


const Matrix& EEBeamColumn2d::getMass(void)
{
    // zero the matrix
    theMatrix.Zero();
    
    // check for quick return
    if (L == 0.0 || rho == 0.0)  {
        return theMatrix;
    }    
    
    double m = 0.5*rho*L;
    theMatrix(0,0) = m;
    theMatrix(1,1) = m;	
    theMatrix(3,3) = m;
    theMatrix(4,4) = m;
    
    return theMatrix; 
}


void EEBeamColumn2d::zeroLoad(void)
{
    theLoad.Zero();
    
    for (int i=0; i<3; i++)  {
        q0[i] = 0.0;
        p0[i] = 0.0;
    }
}


int EEBeamColumn2d::addLoad(ElementalLoad *theLoad, double loadFactor)
{  
    opserr <<"EEBeamColumn2d::addLoad() - "
        << "load type unknown for element: "
        << this->getTag() << endln;
    
    return -1;
}


int EEBeamColumn2d::addInertiaLoadToUnbalance(const Vector &accel)
{
    // check for quick return
    if (L == 0.0 || rho == 0.0)  {
        return 0;
    }    
    
    // get R * accel from the nodes
    const Vector &Raccel1 = theNodes[0]->getRV(accel);
    const Vector &Raccel2 = theNodes[1]->getRV(accel);
    
    if (3 != Raccel1.Size() || 3 != Raccel2.Size())  {
        opserr << "EEBeamColumn2d::addInertiaLoadToUnbalance() - "
            << "matrix and vector sizes are incompatible\n";
        return -1;
    }
    
    // want to add ( - fact * M R * accel ) to unbalance
    // take advantage of lumped mass matrix
    double m = 0.5*rho*L;
    theLoad(0) -= m * Raccel1(0);
    theLoad(1) -= m * Raccel1(1);
    theLoad(3) -= m * Raccel2(0);    
    theLoad(4) -= m * Raccel2(1);    
    
    return 0;
}


const Vector& EEBeamColumn2d::getResistingForce()
{
    theCoordTransf->update();
    
    // zero the residual
    theVector.Zero();
    
    // get measured resisting forces
    static Vector qB(3);
    qB = theSite->getForce();
    
    // add initial forces
    for (int i=0; i<3; i++)  {
        qB(i) += q0[i];
    }
    
    if (iMod == true)  {
        // get measured displacements
        static Vector measDisp(3);
        measDisp = theSite->getDisp();
        
        // correct for displacement control errors using I-Modification
        qB -= kbInit*(measDisp - targDisp);
    } else  {
        // use elastic axial force if axial force from test is zero
        if (qB(0) == 0)  {
            qB(0) = kbInit(0,0) * targDisp(0);
        }
    }
    
    // transform from basic sys B to basic sys A
    q = T^qB;
    
    // Vector for reactions in basic system A
    Vector p0Vec(p0, 3);
    
    // determine resisting forces in global system
    theVector = theCoordTransf->getGlobalResistingForce(q, p0Vec);
    
    // subtract external load
    theVector.addVector(1.0, theLoad, -1.0);
    
    return theVector;
}


const Vector& EEBeamColumn2d::getResistingForceIncInertia()
{	
    theVector = this->getResistingForce();
    
    // add the damping forces if rayleigh damping
    if (alphaM != 0.0 || betaK != 0.0 || betaK0 != 0.0 || betaKc != 0.0)
        theVector += this->getRayleighDampingForces();
    
    // now include the mass portion
    if (L != 0.0 && rho != 0.0)  {
        const Vector &accel1 = theNodes[0]->getTrialAccel();
        const Vector &accel2 = theNodes[1]->getTrialAccel();    
        
        double m = 0.5*rho*L;
        theVector(0) += m * accel1(0);
        theVector(1) += m * accel1(1);
        theVector(3) += m * accel2(0);    
        theVector(4) += m * accel2(1);
    }
    
    return theVector;
}


int EEBeamColumn2d::sendSelf(int commitTag, Channel &theChannel)
{
    // has not been implemented yet.....
    return 0;
}


int EEBeamColumn2d::recvSelf(int commitTag, Channel &theChannel,
                             FEM_ObjectBroker &theBroker)
{
    // has not been implemented yet.....
    return 0;
}


int EEBeamColumn2d::displaySelf(Renderer &theViewer,
                                int displayMode, float fact)
{
    // first determine the end points of the beam based on
    // the display factor (a measure of the distorted image)
    const Vector &end1Crd = theNodes[0]->getCrds();
    const Vector &end2Crd = theNodes[1]->getCrds();	
    
    const Vector &end1Disp = theNodes[0]->getDisp();
    const Vector &end2Disp = theNodes[1]->getDisp();
    
    static Vector v1(3);
    static Vector v2(3);
    
    for (int i = 0; i < 2; i++)  {
        v1(i) = end1Crd(i) + end1Disp(i)*fact;
        v2(i) = end2Crd(i) + end2Disp(i)*fact;    
    }
    
    return theViewer.drawLine (v1, v2, 1.0, 1.0);
}


void EEBeamColumn2d::Print(OPS_Stream &s, int flag)
{
    if (flag == 0)  {
        // print everything
        s << "Element: " << this->getTag(); 
        s << "  type: EEBeamColumn2d  iNode: " << connectedExternalNodes(0);
        s << "  jNode: " << connectedExternalNodes(1) << endln;
        s << "  CoordTransf: " << theCoordTransf->getTag() << endln;
        s << "  mass per unit length:  " << rho << endln;
        s << "  ExperimentalSite, tag: " << theSite->getTag() << endln;
    } else if (flag == 1)  {
        // does nothing
    }
}


Response* EEBeamColumn2d::setResponse(const char **argv, int argc,
    Information &eleInformation, OPS_Stream &output)
{
    Response *theResponse = 0;

    output.tag("ElementOutput");
    output.attr("eleType","EEBeamColumn2d");
    output.attr("eleTag",this->getTag());
    output.attr("node1",connectedExternalNodes[0]);
    output.attr("node2",connectedExternalNodes[1]);

    // global forces
    if (strcmp(argv[0],"force") == 0 || strcmp(argv[0],"forces") == 0 ||
        strcmp(argv[0],"globalForce") == 0 || strcmp(argv[0],"globalForces") == 0)
    {
        output.tag("ResponseType","Px_1");
        output.tag("ResponseType","Py_1");
        output.tag("ResponseType","Mz_1");
        output.tag("ResponseType","Px_2");
        output.tag("ResponseType","Py_2");
        output.tag("ResponseType","Mz_2");

        theResponse = new ElementResponse(this, 2, theVector);
    }
    // local forces
    else if (strcmp(argv[0],"localForce") == 0 || strcmp(argv[0],"localForces") == 0)
    {
        output.tag("ResponseType","N_1");
        output.tag("ResponseType","V_1");
        output.tag("ResponseType","M_1");
        output.tag("ResponseType","N_2");
        output.tag("ResponseType","V_2");
        output.tag("ResponseType","M_2");

        theResponse = new ElementResponse(this, 3, theVector);
    }
    // forces in basic system B
    else if (strcmp(argv[0],"basicForce") == 0 || strcmp(argv[0],"basicForces") == 0)
    {
        output.tag("ResponseType","qB1");
        output.tag("ResponseType","qB2");
        output.tag("ResponseType","qB3");

        theResponse = new ElementResponse(this, 4, Vector(3));
    }
    // deformations in basic system B
    else if (strcmp(argv[0],"deformation") == 0 || strcmp(argv[0],"deformations") == 0 || 
        strcmp(argv[0],"basicDeformation") == 0 || strcmp(argv[0],"basicDeformations") == 0)
    {
        output.tag("ResponseType","vB1");
        output.tag("ResponseType","vB2");
        output.tag("ResponseType","vB3");

        theResponse = new ElementResponse(this, 5, Vector(3));
    }

    output.endTag(); // ElementOutput

    return theResponse;
}


int EEBeamColumn2d::getResponse(int responseID, Information &eleInformation)
{
    double L = theCoordTransf->getInitialLength();
    
    switch (responseID)  {
    case -1:
        return -1;
        
    case 1:  // initial stiffness
        if (eleInformation.theMatrix != 0)  {
            *(eleInformation.theMatrix) = theInitStif;
        }
        return 0;
        
    case 2:  // global forces
        if (eleInformation.theVector != 0)  {
            *(eleInformation.theVector) = this->getResistingForce();
        }
        return 0;      
        
    case 3:  // local forces
        if (eleInformation.theVector != 0)  {
            // Axial
            theVector(0) = -q(0) + p0[0];
            theVector(3) =  q(0);
            // Shear
            theVector(1) =  (q(1)+q(2))/L + p0[1];
            theVector(4) = -(q(1)+q(2))/L + p0[2];
            // Moment
            theVector(2) =  q(1);
            theVector(5) =  q(2);
            
            *(eleInformation.theVector) = theVector;
        }
        return 0;      
        
    case 4:  // forces in basic system B
        if (eleInformation.theVector != 0)  {
            *(eleInformation.theVector) = Tinv^q;
        }
        return 0;      
        
    case 5:  // deformations in basic system B
        if (eleInformation.theVector != 0)  {
            *(eleInformation.theVector) = targDisp;
        }
        return 0;      
        
    default:
        return -1;
    }
}
