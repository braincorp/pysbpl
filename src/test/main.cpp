/*
 * Copyright (c) 2008, Maxim Likhachev
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Carnegie Mellon University nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <cmath>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

#include <sbpl/headers.h>
#include <sbpl/runners.h>


enum MainResultType
{
    INVALID_MAIN_RESULT = -1,

    MAIN_RESULT_SUCCESS = 0,
    MAIN_RESULT_FAILURE = 1,
    MAIN_RESULT_INSUFFICIENT_ARGS = 2,
    MAIN_RESULT_INCORRECT_OPTIONS = 3,
    MAIN_RESULT_UNSUPPORTED_ENV = 4,

    NUM_MAIN_RESULTS
};

/*******************************************************************************
 * PrintUsage - Prints the proper usage of the sbpl test executable.
 *
 * @param argv The command-line arguments; used to determine the name of the
 *             test executable.
 *******************************************************************************/
void PrintUsage(char *argv[])
{
    printf("USAGE: %s [-s] [--env=<env_t>] [--planner=<planner_t>] [--search-dir=<search_t>] <cfg file> [mot prims]\n",
           argv[0]);
    printf("See '%s -h' for help.\n", argv[0]);
}

/*******************************************************************************
 * PrintHelp - Prints a help prompt to the command line when the -h option is
 *             used.
 *
 * @param argv The command line arguments; used to determine the name of the
 *             test executable
 *******************************************************************************/
void PrintHelp(char** argv)
{
    printf("\n");
    printf("Search-Based Planning Library\n");
    printf("\n");
    printf("    %s -h\n", argv[0]);
    printf("    %s [-s] [--env=<env_t>] [--planner=<planner_t>] [--search-dir=<search_t>] <env cfg> [mot prim]\n",
           argv[0]);
    printf("\n");
    printf("[-s]                      (optional) Find a solution for an example navigation\n");
    printf("                          scenario where the robot only identifies obstacles as\n");
    printf("                          it approaches them.\n");
    printf("[--env=<env_t>]           (optional) Select an environment type to choose what\n");
    printf("                          example to run. The default is \"xytheta\".\n");
    printf("<env_t>                   One of 2d, xytheta, xythetamlev, robarm.\n");
    printf("[--planner=<planner_t>]   (optional) Select a planner to use for the example.\n");
    printf("                          The default is \"arastar\".\n");
    printf("<planner_t>               One of arastar, adstar, rstar, anastar.\n");
    printf("[--search-dir=<search_t>] (optional) Select the type of search to run. The default\n");
    printf("                          is \"backwards\".\n");
    printf("<search_t>                One of backward, forward.\n");
    printf("<env cfg>                 Config file representing the environment configuration.\n");
    printf("                          See sbpl/env_examples/ for examples.\n");
    printf("[mot prim]                (optional) Motion primitives file for x,y,theta lattice\n");
    printf("                          planning. See sbpl/matlab/mprim/ for examples.\n");
    printf("                          NOTE: resolution of motion primtives should match that\n");
    printf("                              of the config file.\n");
    printf("                          NOTE: optional use of these for x,y,theta planning is\n");
    printf("                              deprecated.\n");
    printf("\n");
}

/*******************************************************************************
 * CheckIsNavigating
 * @brief Returns whether the -s option is being used.
 *
 * @param numOptions The number of options passed through the command line
 * @param argv The command-line arguments
 * @return whether the -s option was passed in on the cmd line
 *******************************************************************************/
bool CheckIsNavigating(int numOptions, char** argv)
{
    for (int i = 1; i < numOptions + 1; i++) {
        if (strcmp(argv[i], "-s") == 0) {
            return true;
        }
    }
    return false;
}

/*******************************************************************************
 * CheckSearchDirection -
 * @brief Returns the search direction being used
 *
 * @param numOptions The number of options passed through the command line
 * @param argv The command-line arguments
 * @return A string representing the search direction; "backward" by default
 ******************************************************************************/
std::string CheckSearchDirection(int numOptions, char** argv)
{
    int optionLength = strlen("--search-dir=");
    for (int i = 1; i < numOptions + 1; i++) {
        if (strncmp("--search-dir=", argv[i], optionLength) == 0) {
            std::string s(&argv[i][optionLength]);
            return s;
        }
    }
    return std::string("backward");
}

/*******************************************************************************
 * CheckEnvironmentType
 * @brief Returns the environment being used
 *
 * @param numOptions The number of options passed through the command line
 * @param argv The command-line arguments
 * @return A string denoting the environment type; "xytheta" by default
 *******************************************************************************/
std::string CheckEnvironmentType(int numOptions, char** argv)
{
    int optionLength = strlen("--env=");
    for (int i = 1; i < numOptions + 1; i++) {
        if (strncmp("--env=", argv[i], optionLength) == 0) {
            std::string s(&argv[i][optionLength]);
            return s;
        }
    }
    return std::string("xytheta");
}

/*******************************************************************************
 * CheckPlannerType - Checks for a planner specifier passed in through the
 *                    command line. This determines what planner to run in
 *                    the example. If none is found, ARA* is assumed.
 *
 * @param numOptions The number of options passed through the command line
 * @param argv The command-line arguments
 * @return A string denoting the planner type; "arastar" by default
 ******************************************************************************/
std::string CheckPlannerType(int numOptions, char** argv)
{
    int optionLength = strlen("--planner=");
    for (int i = 1; i < numOptions + 1; i++) {
        if (strncmp("--planner=", argv[i], optionLength) == 0) {
            std::string s(&argv[i][optionLength]);
            return s;
        }
    }
    return std::string("arastar");
}

/*******************************************************************************
 * plan2d
 * @brief An example of planning in two-dimensional space without motion primitives.
 *
 * @param plannerType The type of planner to be used in this example
 * @param envCfgFilename The environment config file. See
 *                       sbpl/env_examples/nav2d/ for examples
 * @return 1 if the planner successfully found a solution; 0 otherwise
 *******************************************************************************/
int plan2d(PlannerType plannerType, char* envCfgFilename, bool forwardSearch)
{
    int bRet = 0;
    double allocated_time_secs = 100.0; // in seconds
    double initialEpsilon = 3.0;
    MDPConfig MDPCfg;
    bool bsearchuntilfirstsolution = false;
    bool bforwardsearch = forwardSearch;

    // Initialize Environment (should be called before initializing anything else)
    EnvironmentNAV2D environment_nav2D;
    if (!environment_nav2D.InitializeEnv(envCfgFilename)) {
        throw SBPL_Exception("ERROR: InitializeEnv failed");
    }

    // Initialize MDP Info
    if (!environment_nav2D.InitializeMDPCfg(&MDPCfg)) {
        throw SBPL_Exception("ERROR: InitializeMDPCfg failed");
    }

    // plan a path
    vector<int> solution_stateIDs_V;

    SBPLPlanner* planner = NULL;
    switch (plannerType) {
    case PLANNER_TYPE_ARASTAR:
        printf("Initializing ARAPlanner...\n");
        planner = new ARAPlanner(&environment_nav2D, bforwardsearch);
        break;
    case PLANNER_TYPE_ADSTAR:
        printf("Initializing ADPlanner...\n");
        planner = new ADPlanner(&environment_nav2D, bforwardsearch);
        break;
    case PLANNER_TYPE_RSTAR:
        printf("Initializing RSTARPlanner...\n");
        planner = new RSTARPlanner(&environment_nav2D, bforwardsearch);
        break;
    case PLANNER_TYPE_ANASTAR:
        printf("Initializing anaPlanner...\n");
        planner = new anaPlanner(&environment_nav2D, bforwardsearch);
        break;
    default:
        printf("Invalid planner type\n");
        break;
    }

    // set search mode
    planner->set_search_mode(bsearchuntilfirstsolution);

    if (planner->set_start(MDPCfg.startstateid) == 0) {
        throw SBPL_Exception("ERROR: failed to set start state");
    }

    if (planner->set_goal(MDPCfg.goalstateid) == 0) {
        throw SBPL_Exception("ERROR: failed to set goal state");
    }

    planner->set_initialsolution_eps(initialEpsilon);

    printf("start planning...\n");
    bRet = planner->replan(allocated_time_secs, &solution_stateIDs_V);
    printf("done planning\n");
    std::cout << "size of solution=" << solution_stateIDs_V.size() << std::endl;

    environment_nav2D.PrintTimeStat(stdout);

    const char* sol = "sol.txt";
    FILE* fSol = fopen(sol, "w");
    if (fSol == NULL) {
        throw SBPL_Exception("ERROR: could not open solution file");
    }
    for (unsigned int i = 0; i < solution_stateIDs_V.size(); i++) {
        environment_nav2D.PrintState(solution_stateIDs_V[i], false, fSol);
    }
    fclose(fSol);

    environment_nav2D.PrintTimeStat(stdout);

    //print a path
    if (bRet) {
        //print the solution
        printf("Solution is found\n");
    }
    else {
        printf("Solution does not exist\n");
    }

    fflush(NULL);

    delete planner;

    return bRet;
}

/*******************************************************************************
 * plan2duu
 * @brief An examle of planning in two-dimensional space and under uncertainty.
 *
 * @param plannerType The type of planner to be used in this example
 * @param envCfgFilename The environment config file. See
 *                       sbpl/env_examples/nav2duu/ for examples
 * @return 1 if the planner successfully found a solution; 0 otherwise
 *******************************************************************************/
int plan2duu(PlannerType plannerType, char* envCfgFilename)
{
    int bRet = 0;
    double allocated_time_secs = 10.0; //in seconds
    MDPConfig MDPCfg;

    //Initialize Environment (should be called before initializing anything else)
    EnvironmentNAV2DUU environment_nav2Duu;
    if (!environment_nav2Duu.InitializeEnv(envCfgFilename)) {
        throw SBPL_Exception("ERROR: InitializeEnv failed");
    }

    //Initialize MDP Info
    if (!environment_nav2Duu.InitializeMDPCfg(&MDPCfg)) {
        throw SBPL_Exception("ERROR: InitializeMDPCfg failed");
    }

    //create the planner
    PPCPPlanner planner(&environment_nav2Duu, environment_nav2Duu.SizeofCreatedEnv(), environment_nav2Duu.SizeofH());

    //set start and goal
    if (planner.set_start(MDPCfg.startstateid) == 0) {
        throw SBPL_Exception("ERROR: failed to set start state");
    }
    if (planner.set_goal(MDPCfg.goalstateid) == 0) {
        throw SBPL_Exception("ERROR: failed to set goal state");
    }

    printf("start planning...\n");
    float ExpectedCost, ProbofReachGoal;
    vector<sbpl_PolicyStatewithBinaryh_t> SolutionPolicy;
    bRet = planner.replan(allocated_time_secs, &SolutionPolicy, &ExpectedCost, &ProbofReachGoal);
    printf("done planning\n");

    if (bRet) {
        //print the solution
        printf("Solution is found: exp. cost=%f probofreachgoal=%f\n", ExpectedCost, ProbofReachGoal);
    }
    else {
        printf("Solution does not exist\n");
    }

    fflush(NULL);

    return bRet;
}

/*******************************************************************************
 * planxythetalat
 * @brief An example of planning in three-dimensional space (x,y,theta)
 *
 * @param plannerType The type of planner to be used in this example
 * @param envCfgFilename The environment config file. See
 *                       sbpl/env_examples/nav3d/ for examples
 * @param motPrimFilename The motion primitives file. See
 *                        sbpl/matlab/mprim/ for examples
 * @return 1 if the planner successfully found a solution; 0 otherwise
 *******************************************************************************/
int planxythetalat(PlannerType plannerType, char* envCfgFilename, char* motPrimFilename, bool forwardSearch)
{
    int bRet = 0;
    double allocated_time_secs = 10.0; // in seconds
    double initialEpsilon = 3.0;
    MDPConfig MDPCfg;
    bool bsearchuntilfirstsolution = false;
    bool bforwardsearch = forwardSearch;

    // set the perimeter of the robot (it is given with 0,0,0 robot ref. point for which planning is done)
    vector<sbpl_2Dpt_t> perimeterptsV;
    sbpl_2Dpt_t pt_m;
    double halfwidth = 0.01; //0.3;
    double halflength = 0.01; //0.45;
    pt_m.x = -halflength;
    pt_m.y = -halfwidth;
    perimeterptsV.push_back(pt_m);
    pt_m.x = halflength;
    pt_m.y = -halfwidth;
    perimeterptsV.push_back(pt_m);
    pt_m.x = halflength;
    pt_m.y = halfwidth;
    perimeterptsV.push_back(pt_m);
    pt_m.x = -halflength;
    pt_m.y = halfwidth;
    perimeterptsV.push_back(pt_m);

    // clear the footprint
    perimeterptsV.clear();

    // Initialize Environment (should be called before initializing anything else)
    EnvironmentNAVXYTHETALAT environment_navxythetalat;

    if (!environment_navxythetalat.InitializeEnv(envCfgFilename, perimeterptsV, motPrimFilename)) {
        throw SBPL_Exception("ERROR: InitializeEnv failed");
    }

    // Initialize MDP Info
    if (!environment_navxythetalat.InitializeMDPCfg(&MDPCfg)) {
        throw SBPL_Exception("ERROR: InitializeMDPCfg failed");
    }

    // plan a path
    vector<int> solution_stateIDs_V;

    SBPLPlanner* planner = NULL;
    switch (plannerType) {
    case PLANNER_TYPE_ARASTAR:
        printf("Initializing ARAPlanner...\n");
        planner = new ARAPlanner(&environment_navxythetalat, bforwardsearch);
        break;
    case PLANNER_TYPE_ADSTAR:
        printf("Initializing ADPlanner...\n");
        planner = new ADPlanner(&environment_navxythetalat, bforwardsearch);
        break;
    case PLANNER_TYPE_RSTAR:
        printf("Invalid configuration: xytheta environment does not support rstar planner...\n");
        return 0;
    case PLANNER_TYPE_ANASTAR:
        printf("Initializing anaPlanner...\n");
        planner = new anaPlanner(&environment_navxythetalat, bforwardsearch);
        break;
    default:
        printf("Invalid planner type\n");
        break;
    }

    // set planner properties
    if (planner->set_start(MDPCfg.startstateid) == 0) {
        printf("ERROR: failed to set start state\n");
        throw SBPL_Exception("ERROR: failed to set start state");
    }
    if (planner->set_goal(MDPCfg.goalstateid) == 0) {
        printf("ERROR: failed to set goal state\n");
        throw SBPL_Exception("ERROR: failed to set goal state");
    }
    planner->set_initialsolution_eps(initialEpsilon);
    planner->set_search_mode(bsearchuntilfirstsolution);

    // plan
    printf("start planning...\n");
    bRet = planner->replan(allocated_time_secs, &solution_stateIDs_V);
    printf("done planning\n");
    printf("size of solution=%d\n", (unsigned int)solution_stateIDs_V.size());

    environment_navxythetalat.PrintTimeStat(stdout);

    // write solution to sol.txt
    const char* sol = "sol.txt";
    FILE* fSol = fopen(sol, "w");
    if (fSol == NULL) {
        throw SBPL_Exception("ERROR: could not open solution file");
    }

    // write the discrete solution to file
    for (size_t i = 0; i < solution_stateIDs_V.size(); i++) {
        int x;
        int y;
        int theta;
        environment_navxythetalat.GetCoordFromState(solution_stateIDs_V[i], x, y, theta);
        fprintf(fSol, "%d %d %d\t\t%.3f %.3f %.3f\n", x, y, theta, DISCXY2CONT(x, 0.1), DISCXY2CONT(y, 0.1), DiscTheta2Cont(theta, 16));
    }

    // write the continuous solution to file
    vector<sbpl_xy_theta_pt_t> xythetaPath;
    environment_navxythetalat.ConvertStateIDPathintoXYThetaPath(&solution_stateIDs_V, &xythetaPath);
    printf("solution size=%d\n", (unsigned int)xythetaPath.size());
    for (unsigned int i = 0; i < xythetaPath.size(); i++) {
        fprintf(fSol, "%.3f %.3f %.3f\n", xythetaPath.at(i).x, xythetaPath.at(i).y, xythetaPath.at(i).theta);
    }
    fclose(fSol);

    environment_navxythetalat.PrintTimeStat(stdout);

    // print a path
    if (bRet) {
        // print the solution
        printf("Solution is found\n");
    }
    else {
        printf("Solution does not exist\n");
    }

    fflush(NULL);

    delete planner;

    return bRet;
}

/*******************************************************************************
 * planxythetamlevlat
 * @brief An example of planning with a multiple-level (x,y,theta) lattice
 *
 * @desc An example of planning with a multiple-level x,y,theta lattice (for
 *       example, a base and upper body). There are no additional degrees of
 *       freedom but each level may have a different footprint and should be
 *       checked against a corresponding costmap at its height. Useful for doing
 *       navigation of a tall ground robot operating in a cluttered 3d map.
 * @param plannerType The type of planner to be used in this example
 * @param envCfgFilename The environment config file. See
 *                       sbpl/env_examples/nav3d/ for examples
 * @param motPrimFilename The motion primitives file. See
 *                        sbpl/matlab/mprim/ for examples
 * @return 1 if the planner successfully found a solution; 0 otherwise
 *******************************************************************************/
int planxythetamlevlat(PlannerType plannerType, char* envCfgFilename, char* motPrimFilename, bool forwardSearch)
{
    int bRet = 0;

    double allocated_time_secs = 10.0; //in seconds
    double initialEpsilon = 3.0;
    MDPConfig MDPCfg;
    bool bsearchuntilfirstsolution = false;
    bool bforwardsearch = forwardSearch;

    //set the perimeter of the robot (it is given with 0,0,0 robot ref. point for which planning is done)
    //this is for the default level - base level
    vector<sbpl_2Dpt_t> perimeterptsV;
    sbpl_2Dpt_t pt_m;
    double halfwidth = 0.02; //0.3;
    double halflength = 0.02; //0.45;
    pt_m.x = -halflength;
    pt_m.y = -halfwidth;
    perimeterptsV.push_back(pt_m);
    pt_m.x = halflength;
    pt_m.y = -halfwidth;
    perimeterptsV.push_back(pt_m);
    pt_m.x = halflength;
    pt_m.y = halfwidth;
    perimeterptsV.push_back(pt_m);
    pt_m.x = -halflength;
    pt_m.y = halfwidth;
    perimeterptsV.push_back(pt_m);

    //Initialize Environment (should be called before initializing anything else)
    EnvironmentNAVXYTHETAMLEVLAT environment_navxythetalat;

    if (!environment_navxythetalat.InitializeEnv(envCfgFilename, perimeterptsV, motPrimFilename)) {
        throw SBPL_Exception("ERROR: InitializeEnv failed");
    }

    //this is for the second level - upper body level
    vector<sbpl_2Dpt_t> perimeterptsVV[2];
    perimeterptsVV[0].clear();
    halfwidth = 0.02;
    halflength = 0.02;
    pt_m.x = -halflength;
    pt_m.y = -halfwidth;
    perimeterptsVV[0].push_back(pt_m);
    pt_m.x = halflength;
    pt_m.y = -halfwidth;
    perimeterptsVV[0].push_back(pt_m);
    pt_m.x = halflength;
    pt_m.y = halfwidth;
    perimeterptsVV[0].push_back(pt_m);
    pt_m.x = -halflength;
    pt_m.y = halfwidth;
    perimeterptsVV[0].push_back(pt_m);

    //	perimeterptsV.clear();
    //	perimeterptsVV[0].clear();

    //enable the second level
    int numofaddlevels = 1;
    printf("Number of additional levels = %d\n", numofaddlevels);
    unsigned char cost_inscribed_thresh_addlevels[2]; //size should be at least numofaddlevels
    unsigned char cost_possibly_circumscribed_thresh_addlevels[2]; //size should be at least numofaddlevels
    //no costs are indicative of whether a cell is within inner circle
    cost_inscribed_thresh_addlevels[0] = 255;
    //no costs are indicative of whether a cell is within outer circle
    cost_possibly_circumscribed_thresh_addlevels[0] = 0;
    //no costs are indicative of whether a cell is within inner circle
    cost_inscribed_thresh_addlevels[1] = 255;
    //no costs are indicative of whether a cell is within outer circle
    cost_possibly_circumscribed_thresh_addlevels[1] = 0;
    if (!environment_navxythetalat.InitializeAdditionalLevels(numofaddlevels, perimeterptsVV,
                                                              cost_inscribed_thresh_addlevels,
                                                              cost_possibly_circumscribed_thresh_addlevels))
    {
        std::stringstream ss("ERROR: InitializeAdditionalLevels failed with numofaddlevels=");
        ss << numofaddlevels;
        throw SBPL_Exception(ss.str());
    }

    //set the map for the second level (index parameter for the additional levels and is zero based)
    //for this example, we pass in the same map as the map for the base. In general, it can be a totally different map
    //as it corresponds to a different level
    //NOTE: this map has to have costs set correctly with respect to inner and outer radii of the robot
    //if the second level of the robot has these radii different than at the base level, then costs
    //should reflect this
    //(see explanation for cost_possibly_circumscribed_thresh and
    //cost_inscribed_thresh parameters in environment_navxythetalat.h file)
    int addlevind = 0;
    if (!environment_navxythetalat.Set2DMapforAddLev(
            (const unsigned char**)(environment_navxythetalat.GetEnvNavConfig()->Grid2D), addlevind))
    {
        std::stringstream ss("ERROR: Setting Map for the Additional Level failed with level ");
        ss << addlevind;
        throw SBPL_Exception(ss.str());
    }

    // Initialize MDP Info
    if (!environment_navxythetalat.InitializeMDPCfg(&MDPCfg)) {
        throw SBPL_Exception("ERROR: InitializeMDPCfg failed");
    }

    // plan a path
    vector<int> solution_stateIDs_V;

    SBPLPlanner* planner = NULL;
    switch (plannerType) {
    case PLANNER_TYPE_ARASTAR:
        printf("Initializing ARAPlanner...\n");
        planner = new ARAPlanner(&environment_navxythetalat, bforwardsearch);
        break;
    case PLANNER_TYPE_ADSTAR:
        printf("Initializing ADPlanner...\n");
        planner = new ADPlanner(&environment_navxythetalat, bforwardsearch);
        break;
    case PLANNER_TYPE_RSTAR:
        printf("Invalid configuration: xytheta environment does not support rstar planner...\n");
        return 0;
    case PLANNER_TYPE_ANASTAR:
        printf("Initializing anaPlanner...\n");
        planner = new anaPlanner(&environment_navxythetalat, bforwardsearch);
        break;
    default:
        printf("Invalid planner type\n");
        break;
    }

    if (planner->set_start(MDPCfg.startstateid) == 0) {
        printf("ERROR: failed to set start state\n");
        throw SBPL_Exception("ERROR: failed to set start state");
    }

    if (planner->set_goal(MDPCfg.goalstateid) == 0) {
        printf("ERROR: failed to set goal state\n");
        throw SBPL_Exception("ERROR: failed to set goal state");
    }
    planner->set_initialsolution_eps(initialEpsilon);

    //set search mode
    planner->set_search_mode(bsearchuntilfirstsolution);

    printf("start planning...\n");
    bRet = planner->replan(allocated_time_secs, &solution_stateIDs_V);
    printf("done planning\n");
    std::cout << "size of solution=" << solution_stateIDs_V.size() << std::endl;

    environment_navxythetalat.PrintTimeStat(stdout);

    const char* sol = "sol.txt";
    FILE* fSol = fopen(sol, "w");
    if (fSol == NULL) {
        printf("ERROR: could not open solution file\n");
        throw SBPL_Exception("ERROR: could not open solution file");
    }
    vector<sbpl_xy_theta_pt_t> xythetaPath;
    environment_navxythetalat.ConvertStateIDPathintoXYThetaPath(&solution_stateIDs_V, &xythetaPath);
    printf("solution size=%d\n", (unsigned int)xythetaPath.size());
    for (unsigned int i = 0; i < xythetaPath.size(); i++) {
        fprintf(fSol, "%.3f %.3f %.3f\n", xythetaPath.at(i).x, xythetaPath.at(i).y, xythetaPath.at(i).theta);
    }
    fclose(fSol);

    environment_navxythetalat.PrintTimeStat(stdout);

    //print a path
    if (bRet) {
        //print the solution
        printf("Solution is found\n");
    }
    else {
        printf("Solution does not exist\n");
    }

    fflush(NULL);

    delete planner;

    return bRet;
}

/*******************************************************************************
 * planandnavigate2d
 * @brief An example simulation of how a robot would use two-dimensional
 *        environment planning.
 *
 * @param planner The planner to be used in this example
 * @param envCfgFilename The environment config file. See
 *                       sbpl/env_examples/nav2d/ for examples
 *******************************************************************************/
int planandnavigate2d(PlannerType plannerType, char* envCfgFilename)
{
    double allocated_time_secs_foreachplan = 0.2; //in seconds
    MDPConfig MDPCfg;
    EnvironmentNAV2D environment_nav2D;
    EnvironmentNAV2D trueenvironment_nav2D;
    int size_x = -1, size_y = -1;
    int startx = 0, starty = 0;
    int goalx = -1, goaly = -1;
    const char* sol = "sol.txt";
    FILE* fSol = fopen(sol, "w");
    if (fSol == NULL) {
        throw SBPL_Exception("ERROR: could not open solution file");
    }
    //int dx[8] = {-1, -1, -1,  0,  0,  1,  1,  1};
    //int dy[8] = {-1,  0,  1, -1,  1, -1,  0,  1};
    bool bPrint = false;
    int x, y;
    vector<int> preds_of_changededgesIDV;
    vector<nav2dcell_t> changedcellsV;
    nav2dcell_t nav2dcell;
    unsigned char obsthresh = 0;
    srand(0);
    int plantime_over1secs = 0, plantime_over0p5secs = 0, plantime_over0p1secs = 0, plantime_over0p05secs = 0,
        plantime_below0p05secs = 0;

    //set parameters - should be done before initialization
    if (!trueenvironment_nav2D.SetEnvParameter("is16connected", 1)) {
        throw SBPL_Exception("ERROR: failed to set parameters");
    }
    if (!environment_nav2D.SetEnvParameter("is16connected", 1)) {
        throw SBPL_Exception("ERROR: failed to set parameters");
    }

    //initialize true map and robot map
    if (!trueenvironment_nav2D.InitializeEnv(envCfgFilename)) {
        throw SBPL_Exception("ERROR: InitializeEnv failed");
    }
    trueenvironment_nav2D.GetEnvParms(&size_x, &size_y, &startx, &starty, &goalx, &goaly, &obsthresh);
    unsigned char* map = (unsigned char*)calloc(size_x * size_y, sizeof(unsigned char));

    //print the map
    if (bPrint) printf("true map:\n");
    for (y = 0; bPrint && y < size_y; y++) {
        for (x = 0; x < size_x; x++) {
            printf("%d ", (int)trueenvironment_nav2D.IsObstacle(x, y));
        }
        printf("\n");
    }
    if (bPrint) printf("System Pause (return=%d)\n", system("pause"));

    //Initialize Environment (should be called before initializing anything else)
    if (!environment_nav2D.InitializeEnv(size_x, size_y, map, startx, starty, goalx, goaly, obsthresh)) {
        throw SBPL_Exception("ERROR: InitializeEnv failed");
    }

    //Initialize MDP Info
    if (!environment_nav2D.InitializeMDPCfg(&MDPCfg)) {
        throw SBPL_Exception("ERROR: InitializeMDPCfg failed");
    }

    //create a planner
    vector<int> solution_stateIDs_V;
    bool bforwardsearch = false;

    SBPLPlanner* planner = NULL;
    switch (plannerType) {
    case PLANNER_TYPE_ARASTAR:
        printf("Initializing ARAPlanner...\n");
        planner = new ARAPlanner(&environment_nav2D, bforwardsearch);
        break;
    case PLANNER_TYPE_ADSTAR:
        printf("Initializing ADPlanner...\n");
        planner = new ADPlanner(&environment_nav2D, bforwardsearch);
        break;
    case PLANNER_TYPE_RSTAR:
        printf("Initializing RSTARPlanner...\n");
        planner = new RSTARPlanner(&environment_nav2D, bforwardsearch);
        break;
    case PLANNER_TYPE_ANASTAR:
        printf("Initializing anaPlanner...\n");
        planner = new anaPlanner(&environment_nav2D, bforwardsearch);
        break;
    default:
        printf("Invalid planner type\n");
        break;
    }

    planner->set_initialsolution_eps(2.0);

    //set the start and goal configurations
    if (planner->set_start(MDPCfg.startstateid) == 0) {
        printf("ERROR: failed to set start state\n");
        throw SBPL_Exception("ERROR: failed to set start state");
    }
    if (planner->set_goal(MDPCfg.goalstateid) == 0) {
        printf("ERROR: failed to set goal state\n");
        throw SBPL_Exception("ERROR: failed to set goal state");
    }

    //set search mode
    planner->set_search_mode(false);

    //now comes the main loop
    int goalthresh = 0;
    while (abs(startx - goalx) > goalthresh || abs(starty - goaly) > goalthresh) {

        //simulate sensor data update
        bool bChanges = false;
        preds_of_changededgesIDV.clear();
        changedcellsV.clear();

        int dX = 0;
        int dY = 0;
        for (dX = -2; dX <= 2; dX++) {
            for (dY = -2; dY <= 2; dY++) {
                int x = startx + dX;
                int y = starty + dY;
                if (x < 0 || x >= size_x || y < 0 || y >= size_y) {
                    continue;
                }
                int index = x + y * size_x;
                unsigned char truecost = trueenvironment_nav2D.GetMapCost(x, y);
                if (map[index] != truecost) {
                    map[index] = truecost;
                    environment_nav2D.UpdateCost(x, y, map[index]);
                    printf("setting cost[%d][%d] to %d\n", x, y, map[index]);
                    bChanges = true;
                    // store the changed cells
                    nav2dcell.x = x;
                    nav2dcell.y = y;
                    changedcellsV.push_back(nav2dcell);
                }
            }
        }

        double TimeStarted = clock();

        if (bChanges) {
            if (dynamic_cast<ARAPlanner*> (planner) != NULL) {
                ((ARAPlanner*)planner)->costs_changed(); //use by ARA* planner (non-incremental)
            }
            else if (dynamic_cast<ADPlanner*> (planner) != NULL) {
                //get the affected states
                environment_nav2D.GetPredsofChangedEdges(&changedcellsV, &preds_of_changededgesIDV);
                //let know the incremental planner about them
                ((ADPlanner*)planner)->update_preds_of_changededges(&preds_of_changededgesIDV);
            }
        }
        //planner.force_planning_from_scratch();

        fprintf(fSol, "%d %d ", startx, starty);

        //plan a path
        bool bPlanExists = false;
        while (bPlanExists == false) {
            printf("new planning...\n");
            bPlanExists = (planner->replan(allocated_time_secs_foreachplan, &solution_stateIDs_V) == 1);
            printf("done with the solution of size=%d\n", (unsigned int)solution_stateIDs_V.size());
            environment_nav2D.PrintTimeStat(stdout);
            if (bPlanExists == false) {
                throw SBPL_Exception();
            }

            //for(unsigned int i = 0; i < solution_stateIDs_V.size(); i++) {
            //environment_nav2D.PrintState(solution_stateIDs_V[i], true, fSol);
            //}
            //fprintf(fSol, "*********\n");
        }

        double plantime_secs = (clock() - TimeStarted) / ((double)CLOCKS_PER_SEC);
        fprintf(fSol, "%.5f %.5f\n", plantime_secs, planner->get_solution_eps());
        fflush(fSol);
        if (plantime_secs > 1.0)
            plantime_over1secs++;
        else if (plantime_secs > 0.5)
            plantime_over0p5secs++;
        else if (plantime_secs > 0.1)
            plantime_over0p1secs++;
        else if (plantime_secs > 0.05)
            plantime_over0p05secs++;
        else
            plantime_below0p05secs++;

        //print the map
        int startindex = startx + starty * size_x;
        int goalindex = goalx + goaly * size_x;
        for (y = 0; bPrint && y < size_y; y++) {
            for (x = 0; x < size_x; x++) {
                int index = x + y * size_x;

                //check to see if it is on the path
                bool bOnthePath = false;
                for (int j = 1; j < (int)solution_stateIDs_V.size(); j++) {
                    int newx, newy;
                    environment_nav2D.GetCoordFromState(solution_stateIDs_V[j], newx, newy);
                    if (x == newx && y == newy) bOnthePath = true;
                }

                if (index != startindex && index != goalindex && !bOnthePath)
                    printf("%3d ", map[index]);
                else if (index == startindex)
                    printf("  R ");
                else if (index == goalindex)
                    printf("  G ");
                else if (bOnthePath)
                    printf("  * ");
                else
                    printf("  ? ");
            }
            printf("\n");
        }
        if (bPrint) printf("System Pause (return=%d)\n", system("pause"));

        //move along the path
        if (bPlanExists && (int)solution_stateIDs_V.size() > 1) {
            //get coord of the successor
            int newx, newy;
            environment_nav2D.GetCoordFromState(solution_stateIDs_V[1], newx, newy);

            if (trueenvironment_nav2D.GetMapCost(newx, newy) >= obsthresh) {
                throw SBPL_Exception("ERROR: robot is commanded to move into an obstacle");
            }

            //move
            printf("moving from %d %d to %d %d\n", startx, starty, newx, newy);
            startx = newx;
            starty = newy;

            //update the environment
            environment_nav2D.SetStart(startx, starty);

            //update the planner
            if (planner->set_start(solution_stateIDs_V[1]) == 0) {
                throw SBPL_Exception("ERROR: failed to update robot pose in the planner");
            }
        }

    }

    //print stats
    printf("stats: plantimes over 1 secs=%d; over 0.5 secs=%d; over 0.1 secs=%d; "
           "over 0.05 secs=%d; below 0.05 secs=%d\n",
           plantime_over1secs, plantime_over0p5secs, plantime_over0p1secs, plantime_over0p05secs,
           plantime_below0p05secs);
    fprintf(fSol, "stats: plantimes over 1 secs=%d; over 0.5; secs=%d; over 0.1 secs=%d; "
            "over 0.05 secs=%d; below 0.05 secs=%d\n",
            plantime_over1secs, plantime_over0p5secs, plantime_over0p1secs, plantime_over0p05secs,
            plantime_below0p05secs);

    if (bPrint) printf("System Pause (return=%d)\n", system("pause"));

    fflush(NULL);
    fclose(fSol);

    delete planner;

    return 1;
}



/*******************************************************************************
 * planrobarm - An example of planning a robot arm with six degrees-of-freedom.
 *
 * @param envCfgFilename The environment config file. See
 *                       sbpl/env_examples/robarm for examples
 * @return 1 if the planner successfully found a solution; 0 otherwise
 *******************************************************************************/
int planrobarm(PlannerType plannerType, char* envCfgFilename, bool forwardSearch)
{
    int bRet = 0;
    double allocated_time_secs = 5.0; //in seconds
    MDPConfig MDPCfg;
    bool bforwardsearch = forwardSearch;

    //Initialize Environment (should be called before initializing anything else)
    EnvironmentROBARM environment_robarm;
    if (!environment_robarm.InitializeEnv(envCfgFilename)) {
        throw SBPL_Exception("ERROR: InitializeEnv failed");
    }

    //Initialize MDP Info
    if (!environment_robarm.InitializeMDPCfg(&MDPCfg)) {
        throw SBPL_Exception("ERROR: InitializeMDPCfg failed");
    }

    //srand(1);

    //plan a path
    vector<int> solution_stateIDs_V;

    SBPLPlanner* planner = NULL;
    switch (plannerType) {
    case PLANNER_TYPE_ARASTAR:
        printf("Initializing ARAPlanner...\n");
        planner = new ARAPlanner(&environment_robarm, bforwardsearch);
        break;
    case PLANNER_TYPE_ADSTAR:
        printf("Initializing ADPlanner...\n");
        planner = new ADPlanner(&environment_robarm, bforwardsearch);
        break;
    case PLANNER_TYPE_RSTAR:
        printf("Initializing RSTARPlanner...\n");
        planner = new RSTARPlanner(&environment_robarm, bforwardsearch);
        break;
    case PLANNER_TYPE_ANASTAR:
        printf("Initializing anaPlanner...\n");
        planner = new anaPlanner(&environment_robarm, bforwardsearch);
        break;
    default:
        printf("Invalid planner type\n");
        break;
    }

    if (planner->set_start(MDPCfg.startstateid) == 0) {
        throw SBPL_Exception("ERROR: failed to set start state");
    }

    if (planner->set_goal(MDPCfg.goalstateid) == 0) {
        throw SBPL_Exception("ERROR: failed to set goal state");
    }

    printf("start planning...\n");
    bRet = planner->replan(allocated_time_secs, &solution_stateIDs_V);
    printf("done planning\n");
    std::cout << "size of solution=" << solution_stateIDs_V.size() << std::endl;

    const char* sol = "sol.txt";
    FILE* fSol = fopen(sol, "w");
    if (fSol == NULL) {
        throw SBPL_Exception("ERROR: could not open solution file");
    }
    for (unsigned int i = 0; i < solution_stateIDs_V.size(); i++) {
        environment_robarm.PrintState(solution_stateIDs_V[i], true, fSol);
    }
    fclose(fSol);

    //print a path
    if (bRet) {
        //print the solution
        printf("Solution is found\n");
    }
    else
        printf("Solution does not exist\n");

    fflush(NULL);

    delete planner;

    return bRet;
}

/*******************************************************************************
 * main - Parse command line arguments and launch one of the sbpl examples above.
 *        Launch sbpl with just the -h option for usage help.
 *
 * @param argc The number of command-line arguments
 * @param argv The command-line arguments
 *******************************************************************************/
int main(int argc, char *argv[])
{
    // Print help
    if (argc == 2 && strcmp(argv[1], "-h") == 0) {
        PrintHelp(argv);
        return MAIN_RESULT_SUCCESS;
    }
    else if (argc < 2) {
        PrintUsage(argv);
        return MAIN_RESULT_INSUFFICIENT_ARGS;
    }

    // Find the number of options passed in; cfg and motprim files come after all options
    // options include any string starting with "-"
    int numOptions = 0;
    for (int i = 1; i < argc; i++) {
        if (strncmp("-", argv[i], 1) == 0) {
            numOptions++;
        }
        else {
            break;
        }
    }

    // Check command line arguments to find environment type and whether or not to
    // use one of the navigating examples.
    bool navigating = CheckIsNavigating(numOptions, argv);
    std::string environmentType = CheckEnvironmentType(numOptions, argv);
    std::string plannerType = CheckPlannerType(numOptions, argv);
    std::string searchDir = CheckSearchDirection(numOptions, argv);

    std::cout << "Environment: " << environmentType << "; Planner: " << plannerType << "; Search direction: "
        << searchDir << std::endl;


    int envArgIdx = numOptions + 1;
    EnvironmentType environment = StrToEnvironmentType(environmentType.c_str());
    PlannerType planner = StrToPlannerType(plannerType.c_str());
    bool forwardSearch = !strcmp(searchDir.c_str(), "forward");

    bool usingMotionPrimitives = (argc == numOptions + 3);
    char* motPrimFilename = usingMotionPrimitives ? argv[envArgIdx + 1] : NULL;

    // make sure we've been given valid a valid environment
    if (environment == INVALID_ENV_TYPE || planner == INVALID_PLANNER_TYPE) {
        PrintUsage(argv);
        return MAIN_RESULT_INCORRECT_OPTIONS;
    }

    // Launch the correct example given the planner and an environment file.
    int plannerRes = 0;
    switch (environment) {
    case ENV_TYPE_2D:
        if (navigating) {
            plannerRes = planandnavigate2d(planner, argv[envArgIdx]);
        }
        else {
            plannerRes = plan2d(planner, argv[envArgIdx], forwardSearch);
        }
        break;
    case ENV_TYPE_2DUU:
        printf("Warning: planning in two dimensions under uncertainty is not fully implemented!\n");
        plannerRes = plan2duu(planner, argv[envArgIdx]);
        break;
    case ENV_TYPE_XYTHETA:
        if (navigating) {
            plannerRes = planandnavigatexythetalat(planner, argv[envArgIdx], motPrimFilename, forwardSearch);
        }
        else {
            plannerRes = planxythetalat(planner, argv[envArgIdx], motPrimFilename, forwardSearch);
        }
        break;
    case ENV_TYPE_XYTHETAMLEV:
        plannerRes = planxythetamlevlat(planner, argv[envArgIdx], motPrimFilename, forwardSearch);
        break;
    case ENV_TYPE_ROBARM:
        plannerRes = planrobarm(planner, argv[envArgIdx], forwardSearch);
        break;
    default:
        printf("Unsupported Environment Type...\n");
        PrintUsage(argv);
        return MAIN_RESULT_UNSUPPORTED_ENV;
    }

    return plannerRes == 1 ? MAIN_RESULT_SUCCESS : MAIN_RESULT_FAILURE;
}
