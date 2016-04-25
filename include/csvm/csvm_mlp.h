#ifndef CSVM_MLP_H
#define CSVM_MLP_H

#include <iostream>
#include <vector>
#include "csvm_feature.h"


using namespace std;

namespace csvm{
   
   struct MLPSettings{
      //add your settings variables here (stuff you want to set through the settingsfile)
      int nMLPs;
      int nOutputUnits;
      int nHiddenUnits;
      int nInputUnits;
      int nLayers;
      double learningRate;
      string voting;
      string trainingType;
      int crossValidationInterval;
      double crossValidationSize;
      int epochs;
      double stoppingCriterion;
   };

   class MLPerceptron{
      private:
      //class variables
      MLPSettings settings;
    
      int numPatchesPerSquare;
      
      std::vector<int> layerSizes;

	  std::vector<double> desiredOutput;
	  std::vector<double> votingHistogram;

	  std::vector<vector<double> > biasNodes;
	  std::vector<vector<double> > activations;
	  std::vector<vector<double> > deltas;

	  std::vector<vector<vector<double> > > weights;
	  
	  //private methods
	  
	  //helpMethods:
		void randomizeWeights(std::vector<vector<double> >& array, int indexBottomLayer);
		void setDesiredOutput(Feature f);
		double errorFunction();
		void initializeVectors();
		void checkingSettingsValidity(int actualInputSize);
	  //feedforward:
		double activationFunction(double summedActivation);
		void calculateActivationLayer(int bottomLayer);
		void feedforward();
	  //backpropagation:
		double derivativeActivationFunction(double activationNode);
		void calculateDeltas(int index);
        void outputDelta();
        void hiddenDelta(int index);
        void adjustWeights(int index);
        void backpropgation();
      //voting:
	    void activationsToOutputProbabilities();
		void voting();
		void majorityVoting();
		void sumVoting();
		unsigned int mostVotedClass();
	  //training:
	    void training(vector<Feature>& randomFeatures,vector<Feature>& validationSet);	
	    vector<Feature>& normalizeInput(vector<Feature>& allInputFeatures); 	
        void crossvaldiation(vector<Feature>& randomFeatures,vector<Feature>& validationSet);
		bool isErrorOnValidationSetLowEnough(vector<Feature>& validationSet);
	    
   public:
      void setSettings(MLPSettings s);
      void train(vector<Feature>& randomFeatures,vector<Feature>& validationSet, int numPatchSquare);
      unsigned int classify(vector<Feature> imageFeatures);
      vector<double> classifyPooling(vector<Feature> imageFeatures);
   };
      
}
#endif
