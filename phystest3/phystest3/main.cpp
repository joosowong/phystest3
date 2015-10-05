/*
=====================================================================

Book				: Learning Physics Modeling with PhysX (ISBN: 978-1-84969-814-6)
Author				: Krishna Kumar
Compiler used		: Visual C++ 2010 Express
PhysX SDK version	: 3.3.0 
Source code name	: ch4_1_CollisionDetection
Reference Chapter	: Chapter-4: Collision Detection

Description			: This example demonstrates how to use simulation event callbacks like- onTrigger() and onContact().
					  These simulation events can be received by inheriting the �PxSimulationEventCallback� class. 
					  In this example 'SimulationEvents' class inherits from� PxSimulationEventCallback� class.
					  Once you created a derived class from �PxSimulationEventCallback� class, an instance of it is 
					  used to register the simulation callback in PhysX. A customized FilterShader is also used in this
					  program. Results of simulation events are printed on the glut console window. This program also 
					  shows how to enable CCD (Continuous Collision Detection) for a ridig body. 
					   
					  Navigation Controls:-
					  Mouse Left-Button Drag  : Scene orbital navigation
					  Mouse Right-Button Drag : Scene zoom-in/zoom-out
				
=====================================================================
*/



#include <iostream> 
#include <PxPhysicsAPI.h> //Single header file to include all features of PhysX API 
#include <GL/freeglut.h>  //OpenGL window tool kit 

#include "RenderBuffer.h"	  //Used for rendering PhysX objetcs 
#include "SimulationEvents.h" //Used for receiving simulation events

//-------Loading PhysX libraries (32bit only)----------//

#ifdef _DEBUG //If in 'Debug' load libraries for debug mode 
#pragma comment(lib, "PhysX3DEBUG_x86.lib")				//Always be needed  
#pragma comment(lib, "PhysX3CommonDEBUG_x86.lib")		//Always be needed
#pragma comment(lib, "PhysX3ExtensionsDEBUG.lib")		//PhysX extended library 

#else //Else load libraries for 'Release' mode
#pragma comment(lib, "PhysX3_x86.lib")	
#pragma comment(lib, "PhysX3Common_x86.lib") 
#pragma comment(lib, "PhysX3Extensions.lib")
#endif

using namespace std;
using namespace physx; 


//========== Global variables ============//

int gWindowWidth  = 800; //Screen width
int gWindowHeight = 600; //Screen height

float pi = 3.14159265358979323846;

//---Scene navigation----
int gOldMouseX = 0;
int gOldMouseY = 0;

bool isMouseLeftBtnDown  = false;
bool isMouseRightBtnDown = false;

float gCamRoateX	= 15; 
float gCamRoateY	= 0;
float gCamDistance	= -30;
//------------------------


static PxPhysics*				gPhysicsSDK = NULL;			//Instance of PhysX SDK
static PxFoundation*			gFoundation = NULL;			//Instance of singleton foundation SDK class
static PxDefaultErrorCallback	gDefaultErrorCallback;		//Instance of default implementation of the error callback
static PxDefaultAllocator		gDefaultAllocatorCallback;	//Instance of default implementation of the allocator interface required by the SDK
PxScene*						gScene = NULL;				//Instance of PhysX Scene				
PxReal							gTimeStep = 1.0f/60.0f;		//Time-step value for PhysX simulation 

static SimulationEvents gSimulationEventCallback;			//Instance of 'SimulationEvents' class inherited from 'PxSimulationEventCallback' class




//========== PhysX function prototypes ===========//

void InitPhysX();		//Initialize the PhysX SDK and create actors. 
void StepPhysX();		//Step PhysX simulation
void ShutdownPhysX();	//Shutdown PhysX SDK
void countActor(); //Count the actors



//Functions for glut callbacks
void OnRender();					//Display callback for the current glut window
void OnIdle();						//Called whenever the application is idle
void OnReshape(int, int);			//Called whenever the application window is resized
void OnShutdown();					//Called on application exit
void OnMouseMotion(int,int);		//Called when the mouse is moving
void OnMousePress(int,int,int,int); //Called when any mouse button is pressed

/* commented so I can test my very own version
//Defining a custome filter shader 
PxFilterFlags customFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0, 
										PxFilterObjectAttributes attributes1, PxFilterData filterData1,
										PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
	// all initial and persisting reports for everything, with per-point data
	pairFlags = PxPairFlag::eCONTACT_DEFAULT
			  |	PxPairFlag::eTRIGGER_DEFAULT 
			  | PxPairFlag::eNOTIFY_CONTACT_POINTS;
//			  | PxPairFlag::eCCD_LINEAR; //Set flag to enable CCD (Continuous Collision Detection) 
	
	return PxFilterFlag::eDEFAULT;		
}
*/

//Defining my very own super super custom filter
PxFilterFlags customFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0, 
										PxFilterObjectAttributes attributes1, PxFilterData filterData1,
										PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)

{
		pairFlags = PxPairFlag::eCONTACT_DEFAULT;
		return PxFilterFlag::eDEFAULT;
}



void main(int argc, char** argv) 
{

	glutInit(&argc, argv);								//Initialize GLUT
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);		//Enable double buffering
	glutInitWindowSize(gWindowWidth, gWindowHeight);	//Set window's initial width & height
	
	glutCreateWindow("titotatetu"); // Create a window with the given title

	InitPhysX();
	
	glutDisplayFunc(OnRender);	//Display callback for the current glut window
	glutIdleFunc(OnIdle);		//Called whenever the application is idle
	glutReshapeFunc(OnReshape); //Called whenever the app window is resized

	glutMouseFunc(OnMousePress);	//Called on mouse button event
	glutMotionFunc(OnMouseMotion);	//Called on mouse motion event 

	glutMainLoop();				//Enter the event-processing loop
	atexit(OnShutdown);			//Called on application exit 
}


void InitPhysX() 
{
	//Creating foundation for PhysX
	gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback);
	
	//Creating instance of PhysX SDK
	gPhysicsSDK = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale() );

	if(gPhysicsSDK == NULL) 
	{
		cerr<<"Error creating PhysX3 device, Exiting..."<<endl;
		exit(1);
	}



	//Creating scene
	PxSceneDesc sceneDesc(gPhysicsSDK->getTolerancesScale());		//Descriptor class for scenes 

	sceneDesc.gravity		= PxVec3(0.0f, -9.8f, 0.0f);			//Setting gravity
	//sceneDesc.gravity		= PxVec3(0.0f, 0.0f, 0.0f);			//Setting gravity
	sceneDesc.cpuDispatcher = PxDefaultCpuDispatcherCreate(1);		//Creates default CPU dispatcher for the scene
	
	sceneDesc.filterShader  = customFilterShader;					//Creates custom user collision filter shader for the scene
	sceneDesc.simulationEventCallback = &gSimulationEventCallback;  //Resgistering for receiving simulation events
	
	sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;					//Set flag to enable CCD (Continuous Collision Detection) 
	
	gScene = gPhysicsSDK->createScene(sceneDesc);					//Creates a scene 

	
	
	//This will enable basic visualization of PhysX objects like- actors collision shapes and their axes. 
	//The function PxScene::getRenderBuffer() is used to render any active visualization for scene.
	gScene->setVisualizationParameter(PxVisualizationParameter::eSCALE,				1.0);	//Global visualization scale which gets multiplied with the individual scales
	gScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES,	1.0f);	//Enable visualization of actor's shape
	gScene->setVisualizationParameter(PxVisualizationParameter::eACTOR_AXES,		1.0f);	//Enable visualization of actor's axis
	
	
	//Creating PhysX material (staticFriction, dynamicFriction, restitution)
	PxMaterial* material = gPhysicsSDK->createMaterial(0.5f,0.5f,0.5f);

	
	
	//---------Creating actors-----------]
	
	//1-Creating static plane that will act as ground	 
	PxTransform planePos =	PxTransform(PxVec3(0.0f),PxQuat(PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f)));	//Position and orientation(transform) for plane actor  
	PxRigidStatic* plane =  gPhysicsSDK->createRigidStatic(planePos);								//Creating rigid static actor	
							plane->createShape(PxPlaneGeometry(), *material);						//Defining geometry for plane actor
							gScene->addActor(*plane);												//Adding plane actor to PhysX scene


		
	{	
		//Creating a dynamic sphere (It will fall on to trigger shape which will invoke 'onTrigger()' function)
		PxTransform			spherePos(PxVec3(-10.0f, 10.0f, 0.0f));											
		PxSphereGeometry	sphereGeometry(2);										
		PxRigidDynamic*		sphere = PxCreateDynamic(*gPhysicsSDK, spherePos, sphereGeometry, *material, 1.0f);		
							gScene->addActor(*sphere);
	
		//Creating a trigger shape(trigger shape can't collide against any abject, 
		//thus it is made static otherwise it will fall under the effect of gravity) 
		PxTransform		boxPos(PxVec3(-10.0f, 2.10f, 0.0f));												
		PxShape*		boxShape =	gPhysicsSDK->createShape(PxBoxGeometry(PxVec3(3.0f,2.0f,3.0f)),*material);
						
						boxShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);	//flagged to disable shape collision
						boxShape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);		//flagged as trigger shape

		PxRigidStatic*	gBox = PxCreateStatic(*gPhysicsSDK, boxPos, *boxShape);
						gScene->addActor(*gBox);														
	}


		
	{	
		//Creating a rigid dynamic sphere (It will fall on to the other rigid dynamic body which will invoke 'onContact()' function)																			 
/*		PxTransform			spherePos(PxVec3(10.0f, 10.0f, 0.0f));											
		PxSphereGeometry	sphereGeometry(2);										
		PxRigidDynamic*		sphere = PxCreateDynamic(*gPhysicsSDK, spherePos, sphereGeometry, *material, 1.0f);
							
							sphere->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true); //Set flag to enable CCD (Continuous Collision Detection) for the actor
							
							gScene->addActor(*sphere);
*/
	
		int numberofblocks=100;
		//PxVec3 offset = PxVec3(0,0,-1);
		PxReal radius=1;
		PxReal height=1;
		PxVec3 offset0 = PxVec3(-height,0,0);
		PxVec3 offset1 = PxVec3(height,0,0);
		PxVec3 initpos = PxVec3(10.0f, 100.0f, -70.0f);

		PxTransform		boxPos1(initpos,PxQuat(PxHalfPi, PxVec3(0.0f, 1.0f, 0.0f)));	//Position and orientation(transform) for box actor 
		PxRigidDynamic	*gBoxOri = NULL;				//Instance of box actor 
		PxCapsuleGeometry	sphereGeometry(radius,height);											//Defining geometry for box actor
		gBoxOri = PxCreateDynamic(*gPhysicsSDK, boxPos1, sphereGeometry, *material, 1.0f);		//Creating rigid static actor
		gScene->addActor(*gBoxOri);														//Adding box actor to PhysX scene

		for (PxU32 i=1; i<numberofblocks; i++)
		{
			PxTransform		boxPos1(initpos+PxVec3(0.0f, 0.0f, 2*i*height/*2.0f*/),PxQuat(PxHalfPi, PxVec3(0.0f, 1.0f, 0.0f)));												//Position and orientation(transform) for box actor 
			PxRigidDynamic	*gBox = NULL;				//Instance of box actor 
			PxCapsuleGeometry	sphereGeometry(radius,height);											//Defining geometry for box actor
							gBox = PxCreateDynamic(*gPhysicsSDK, boxPos1, sphereGeometry, *material, 1.0f);		//Creating rigid static actor
							gScene->addActor(*gBox);														//Adding box actor to PhysX scene

							// adding some joint to the mix
			//PxSphericalJoint* joint = PxSphericalJointCreate(*gPhysicsSDK, gBoxOri, PxTransform(-offset), gBox, PxTransform(offset));
			//PxSphericalJoint* joint = PxSphericalJointCreate(*gPhysicsSDK, gBoxOri, PxTransform(offset0), gBox, PxTransform(offset1)); //uncomment to use spherical joints
			PxD6Joint* joint = PxD6JointCreate(*gPhysicsSDK, gBoxOri, PxTransform(offset0), gBox, PxTransform(offset1));

			//add some limits to the joint
			//joint->setLimitCone(PxJointLimitCone(0.00000001f,0.00000001f,0.1f));
			//joint->setSphericalJointFlag(PxSphericalJointFlag::eLIMIT_ENABLED,true);
			joint->setSwingLimit(PxJointLimitCone(1.57f,1.57f,0.00000001f));
			joint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);
			joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLOCKED);
			joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLIMITED);
			joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLIMITED);
			//joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE); //free to rotate around y axis

			gBoxOri=gBox;
			//cout<< i <<"\n";
		}
		//Creating a rigid dynamic box resting on static plane 																 
		PxTransform		boxPos(PxVec3(10.0f, 2.0f, 0.0f));											
		PxBoxGeometry	boxGeometry(PxVec3(30.0f,2.0f,30.0f));										
		PxRigidDynamic* box2 = PxCreateDynamic(*gPhysicsSDK, boxPos, boxGeometry, *material, 1.0f);		
						gScene->addActor(*box2);
	}
}

//define langevin

void countActor(void)
{
    PxU32 nbActors = gScene->getNbActors(physx::PxActorTypeSelectionFlag::eRIGID_DYNAMIC);
	PxActor** actors = new PxActor*[nbActors];
	gScene->getActors(physx::PxActorTypeSelectionFlag::eRIGID_DYNAMIC,actors,nbActors);
 
	/*
	while(nbActors--)
    {
		cout<<"nb"<<nbActors<<"_"<<"actor"<<actors<<"\n";
    }
	*/
	/*
	for(int init=0 ; init<=nbActors ; init++)
	{
		cout<<"actor"<<actors[init]<<"\n";
	}
	*/
}

/*float viscosity=viscosity=6.6e-3; /// nucleoplasme change for other medium !
float friction=6.*pi*viscosity*20.*pow(10.,-9.);

PxVec3 langevin(simactor,float sigma,float friction)
{
	argvb=simactor.getLinearVelocity
	return(PxVec3(0.1f,0.1f,0.1f));
}*/

/*python langevin
#Arg1: corps, arg2: sigma, arg3: friction
def langevin_tr(arg1,arg2,arg3):
	argvb=arg1.getLinearVel()
	argfx=2.*arg2*(.5-random())-arg3*argvb[0]
	argfy=2.*arg2*(.5-random())-arg3*argvb[1]
	argfz=2.*arg2*(.5-random())-arg3*argvb[2]
	arg1.addForce((argfx,argfy,argfz))
*/


void StepPhysX()					//Stepping PhysX
{ 

	gScene->simulate(gTimeStep);	//Advances the simulation by 'gTimeStep' time
	gScene->fetchResults(true);		//Block until the simulation run is completed
} 


void ShutdownPhysX()				//Shutdown PhysX
{
	gPhysicsSDK->release();			//Removes any actors,  particle systems, and constraint shaders from this scene
	gFoundation->release();			//Destroys the instance of foundation SDK
}




void OnRender() 
{
	//Update PhysX	
	if(gScene) 
		StepPhysX();
		countActor();

	glClear(GL_COLOR_BUFFER_BIT);
	glLoadIdentity();
	
	glTranslatef(0,0,gCamDistance);
	glRotatef(gCamRoateX,1,0,0);
	glRotatef(gCamRoateY,0,1,0);
	
	RenderData(gScene->getRenderBuffer());

	glutSwapBuffers();
}


void OnReshape(int w, int h) 
{
	glViewport(0,0,w,h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, (GLfloat)w / (GLfloat)h, 0.1f, 100000.0f);
	glMatrixMode(GL_MODELVIEW);
}

void OnIdle() 
{
	glutPostRedisplay();
}

void OnShutdown() 
{
	ShutdownPhysX();
}



void OnMouseMotion(int curMouseX, int curMouseY) 
{
	if(isMouseLeftBtnDown)
	{
		 gCamRoateY += (curMouseX - gOldMouseX)/5.0f;
		 gCamRoateX += (curMouseY - gOldMouseY)/5.0f;
	}

	if(isMouseRightBtnDown)
	{
		gCamDistance -= (curMouseY - gOldMouseY)/5.0f;
	}

	gOldMouseX = curMouseX;
	gOldMouseY = curMouseY;

}


void OnMousePress(int mouseBtn, int mouseBtnState, int curMouseX, int curMouseY)
{
	if (mouseBtnState == GLUT_DOWN) 
	{
		if(mouseBtn== GLUT_LEFT_BUTTON) 
			isMouseLeftBtnDown = true;
		
		else if(mouseBtn == GLUT_RIGHT_BUTTON)
			isMouseRightBtnDown = true;
		
		gOldMouseX = curMouseX;
		gOldMouseY = curMouseY;

	}

	if (mouseBtnState == GLUT_UP ) 
	{
	  	isMouseLeftBtnDown = false;
		isMouseRightBtnDown = false;
	}

	//cout<<mouseBtn<<" "<<mouseBtnState<<" "<<x<<"|"<<y<<"\n";
}
