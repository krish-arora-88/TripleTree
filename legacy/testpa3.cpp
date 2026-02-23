/**
 * @file main.cpp
 * @description basic test cases for TripleTree
 *              CPSC 221 PA3
 *
 *              You may add your own tests to this file
 *
 *              THIS FILE WILL NOT BE SUBMITTED
 */

#define IMAGE_1 "green-1x1"
#define IMAGE_2 "rgb-3x1"
#define IMAGE_3 "mix-3x3"
#define IMAGE_4 "mix-2x5"
#define IMAGE_5 "pruneto16leaves-8x5"
#define IMAGE_6 "malachi-60x87"

#include <iostream>
#include <string>

#include "tripletree.h"

using namespace std;

/**********************************/
/*** TEST FUNCTION DECLARATIONS ***/
/**********************************/
void TestBuildRender(int image_num);
void TestFlipHorizontal(int image_num);
void TestRotateCCW(int image_num);
void TestPrune(double tol);

// You should probably write tests for your copy constructor / operator=
// and tests which combine flip/rotate/prune

/***********************************/
/*** MAIN FUNCTION PROGRAM ENTRY ***/
/***********************************/

int main(int argc, char* argv[]) {

	// provide one command-line argument as a number in the range of [1, 6] to specify the test image used
	int image_number = 1; // default image number
	// set image_number from first command-line argument
	if (argc > 1)
		image_number = atoi(argv[1]);
	// clamp image_number to allowable range (change these if you add your own images)
	if (image_number < 1)
		image_number = 1;
	if (image_number > 6)
		image_number = 6;

	TestBuildRender(image_number);
	TestFlipHorizontal(image_number);
	TestRotateCCW(image_number);
	TestPrune(0.1);

	return 0;
}

/*************************************/
/*** TEST FUNCTION IMPLEMENTATIONS ***/
/*************************************/

void TestBuildRender(int image_num) {
	cout << "Entered TestBuildRender" << endl;

	// read input PNG
	string input_path = "images-original/";
	string output_path = "images-output/";
	switch (image_num) {
		case 1:
			input_path = input_path + IMAGE_1 + ".png";
			output_path = output_path + IMAGE_1 + "-render.png";
			break;
		case 2:
			input_path = input_path + IMAGE_2 + ".png";
			output_path = output_path + IMAGE_2 + "-render.png";
			break;
		case 3:
			input_path = input_path + IMAGE_3 + ".png";
			output_path = output_path + IMAGE_3 + "-render.png";
			break;
		case 4:
			input_path = input_path + IMAGE_4 + ".png";
			output_path = output_path + IMAGE_4 + "-render.png";
			break;
		case 5:
			input_path = input_path + IMAGE_5 + ".png";
			output_path = output_path + IMAGE_5 + "-render.png";
			break;
		case 6:
			input_path = input_path + IMAGE_6 + ".png";
			output_path = output_path + IMAGE_6 + "-render.png";
			break;
		default:
			input_path = input_path + IMAGE_6 + ".png";
			output_path = output_path + IMAGE_6 + "-render.png";
			break;
	}
	PNG input;
	input.readFromFile(input_path);

	cout << "Constructing TripleTree from image... ";
	TripleTree t(input);
	cout << "done." << endl;

	cout << "Rendering tree to PNG... ";
	PNG output = t.Render();
	cout << "done." << endl;

	// write output PNG
	cout << "Writing rendered PNG to file... ";
	output.writeToFile(output_path);
	cout << "done." << endl;

	cout << "Exiting TestBuildRender.\n" << endl;
}

void TestFlipHorizontal(int image_num) {
	cout << "Entered TestFlipHorizontal" << endl;

	// read input PNG
	string input_path = "images-original/";
	string output_path = "images-output/";
	switch (image_num) {
	case 1:
		input_path = input_path + IMAGE_1 + ".png";
		output_path = output_path + IMAGE_1;
		break;
	case 2:
		input_path = input_path + IMAGE_2 + ".png";
		output_path = output_path + IMAGE_2;
		break;
	case 3:
		input_path = input_path + IMAGE_3 + ".png";
		output_path = output_path + IMAGE_3;
		break;
	case 4:
		input_path = input_path + IMAGE_4 + ".png";
		output_path = output_path + IMAGE_4;
		break;
	case 5:
		input_path = input_path + IMAGE_5 + ".png";
		output_path = output_path + IMAGE_5;
		break;
	case 6:
		input_path = input_path + IMAGE_6 + ".png";
		output_path = output_path + IMAGE_6;
		break;
	default:
		input_path = input_path + IMAGE_6 + ".png";
		output_path = output_path + IMAGE_6;
		break;
	}
	PNG input;
	input.readFromFile(input_path);

	cout << "Constructing TripleTree from image... ";
	TripleTree t(input);
	cout << "done." << endl;

	cout << "Calling FlipHorizontal... ";
	t.FlipHorizontal();
	cout << "done." << endl;

	cout << "Rendering tree to PNG... ";
	PNG output = t.Render();
	cout << "done." << endl;

	// write output PNG
	cout << "Writing rendered PNG to file... ";
	output.writeToFile(output_path + "-fh-render.png");
	cout << "done." << endl;

	cout << "Calling FlipHorizontal a second time... ";
	t.FlipHorizontal();
	cout << "done." << endl;

	cout << "Rendering tree to PNG... ";
	output = t.Render();
	cout << "done." << endl;

	// write output PNG
	cout << "Writing rendered PNG to file... ";
	output.writeToFile(output_path + "-fh_x2-render.png");
	cout << "done." << endl;

	cout << "Exiting TestFlipHorizontal.\n" << endl;
}

void TestRotateCCW(int image_num) {
	cout << "Entered TestRotateCCW" << endl;

	// read input PNG
	string input_path = "images-original/";
	string output_path = "images-output/";
	switch (image_num) {
	case 1:
		input_path = input_path + IMAGE_1 + ".png";
		output_path = output_path + IMAGE_1;
		break;
	case 2:
		input_path = input_path + IMAGE_2 + ".png";
		output_path = output_path + IMAGE_2;
		break;
	case 3:
		input_path = input_path + IMAGE_3 + ".png";
		output_path = output_path + IMAGE_3;
		break;
	case 4:
		input_path = input_path + IMAGE_4 + ".png";
		output_path = output_path + IMAGE_4;
		break;
	case 5:
		input_path = input_path + IMAGE_5 + ".png";
		output_path = output_path + IMAGE_5;
		break;
	case 6:
		input_path = input_path + IMAGE_6 + ".png";
		output_path = output_path + IMAGE_6;
		break;
	default:
		input_path = input_path + IMAGE_6 + ".png";
		output_path = output_path + IMAGE_6;
		break;
	}
	PNG input;
	input.readFromFile(input_path);

	cout << "Constructing TripleTree from image... ";
	TripleTree t(input);
	cout << "done." << endl;

	cout << "Calling RotateCCW... ";
	t.RotateCCW();
	cout << "done." << endl;

	cout << "Rendering tree to PNG... ";
	PNG output = t.Render();
	cout << "done." << endl;

	// write output PNG
	cout << "Writing rendered PNG to file... ";
	output.writeToFile(output_path + "-rccw_x1-render.png");
	cout << "done." << endl;

	cout << "Calling RotateCCW a second time... ";
	t.RotateCCW();
	cout << "done." << endl;

	cout << "Rendering tree to PNG... ";
	output = t.Render();
	cout << "done." << endl;

	// write output PNG
	cout << "Writing rendered PNG to file... ";
	output.writeToFile(output_path + "-rccw_x2-render.png");
	cout << "done." << endl;

	cout << "Calling RotateCCW a third time... ";
	t.RotateCCW();
	cout << "done." << endl;

	cout << "Rendering tree to PNG... ";
	output = t.Render();
	cout << "done." << endl;

	// write output PNG
	cout << "Writing rendered PNG to file... ";
	output.writeToFile(output_path + "-rccw_x3-render.png");
	cout << "done." << endl;

	cout << "Calling RotateCCW a fourth time... ";
	t.RotateCCW();
	cout << "done." << endl;

	cout << "Rendering tree to PNG... ";
	output = t.Render();
	cout << "done." << endl;

	// write output PNG
	cout << "Writing rendered PNG to file... ";
	output.writeToFile(output_path + "-rccw_x4-render.png");
	cout << "done." << endl;

	cout << "Exiting TestRotateCCW.\n" << endl;
}

void TestPrune(double tol) {
	cout << "Entered TestPrune, tolerance: " << tol << endl;

	// read input PNG
	PNG input;
	input.readFromFile("images-original/pruneto16leaves-8x5.png");

	cout << "Constructing TripleTree from image... ";
	TripleTree t(input);
	cout << "done." << endl;

	cout << "Tree contains " << t.NumLeaves() << " leaves." << endl;

	cout << "Calling Prune... ";
	t.Prune(tol);
	cout << "done." << endl;

	cout << "Pruned tree contains " << t.NumLeaves() << " leaves." << endl;

	cout << "Rendering tree to PNG... ";
	PNG output = t.Render();
	cout << "done." << endl;

	// write output PNG
	string output_path = "images-output/pruneto16leaves-8x5-prune_" + to_string(tol) + "-render.png";
	cout << "Writing rendered PNG to file... ";
	output.writeToFile(output_path);
	cout << "done." << endl;

	cout << "Exiting TestPrune.\n" << endl;
}