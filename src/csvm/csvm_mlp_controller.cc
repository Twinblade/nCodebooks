#include <csvm/csvm_mlp_controller.h>
 
/* This class will control the multiple MLPs for mlp pooling
 * 
 */
 
 using namespace std;
 using namespace csvm;
 
MLPController::MLPController(FeatureExtractor* fe, ImageScanner* imScan, CSVMSettings* se, CSVMDataset* ds){
	featExtr = *fe;
	imageScanner = *imScan;
	settings = *se;
	dataset = *ds;
}
//--------------start: init MLP's-------------------
void MLPController::setSettings(MLPSettings s){
	cout << "settings set" << std::endl;

	nHiddenBottomLevel = s.nHiddenUnits;
	
	//init global variables	
	nMLPs = pow(s.nSplitsForPooling,2);
	validationSize = dataset.getTrainSize()*s.crossValidationSize;
	trainSize = dataset.getTrainSize() - validationSize;

	for(int i=0;i<2;i++){
		minValues.push_back(1000);
		maxValues.push_back(0);
	}
	
	//reserve global vectors
	numPatchesPerSquare.reserve(nMLPs);
	mlps.reserve(s.stackSize);
	
	mlps = vector<vector<MLPerceptron> >(nMLPs);
	
	for(int j = 0; j < nMLPs;j++){
		MLPerceptron mlp;
		mlp.setSettings(s);
		mlps[0].push_back(mlp);
	}
	
	//settings second parameter	
	MLPerceptron mlp;
	s.nInputUnits = nHiddenBottomLevel * 4;
	s.nHiddenUnits = s.nHiddenUnitsFirstLayer;//find parameter
	mlp.setSettings(s);
	mlps[1].push_back(mlp);
}

void MLPController::setMinAndMaxValueNorm(vector<Feature>& inputFeatures, int index){
	minValues[index] = inputFeatures[0].content[0];
	maxValues[index] = inputFeatures[0].content[0];

	//compute min and max of all the inputs	
	for(unsigned int i = 0; i < inputFeatures.size();i++){
		double possibleMaxValue = *std::max_element(inputFeatures[i].content.begin(), inputFeatures[i].content.end());
		double possibleMinValue = *std::min_element(inputFeatures[i].content.begin(), inputFeatures[i].content.end()); 
	
		if(possibleMaxValue > maxValues[index])
			maxValues[index] = possibleMaxValue;
			
		if(possibleMinValue < minValues[index])
			minValues[index] = possibleMinValue;
	}
}

vector<Feature>& MLPController::normalizeInput(vector<Feature>& inputFeatures, int index){
	if (maxValues[index] - minValues[index] != 0){
		//normalize all the inputs
		for(unsigned int i = 0; i < inputFeatures.size();i++){
			for(int j = 0; j < inputFeatures[i].size;j++)
				inputFeatures[i].content[j] = (inputFeatures[i].content[j] - minValues[index])/(maxValues[index] - minValues[index]);
		}
	}else{
		for(unsigned int i = 0; i<inputFeatures.size();i++){
			for(int j = 0; j < inputFeatures[i].size;j++)
				inputFeatures[i].content[j] = 0;
		}
	}
	return inputFeatures;		
}

int MLPController::calculateSquareOfPatch(Patch patch){
	int splits = settings.mlpSettings.nSplitsForPooling;
	
	int middlePatchX = patch.getX() + patch.getWidth() / 2;
	int middlePatchY = patch.getY() + patch.getHeight() / 2;
	
	int imWidth  = settings.datasetSettings.imWidth;
	int imHeight = settings.datasetSettings.imHeight;
	
	return middlePatchX / (imWidth/splits) + (splits * (middlePatchY / (imHeight/splits)));
}
//-------------------end: init MLP's--------------------------
//-------------------start: data creation methods-------------
vector<Feature>& MLPController::createCompletePictureSet(vector<Feature>& validationSet,int start,int end){
	vector<Patch> patches;   
	for(int i = start; i < end;i++){	
		Image* im = dataset.getTrainImagePtr(i);
		 
		//extract patches
		patches = imageScanner.scanImage(im);
      
		//extract features from all patches
		for(size_t patch = 0; patch < patches.size(); ++patch){
			Feature newFeat = featExtr.extract(patches[patch]);
			newFeat.setSquareId(calculateSquareOfPatch(patches[patch]));
			validationSet.push_back(newFeat);
		}
	}
	return validationSet;
}

vector<vector<Feature> > MLPController::splitUpDataBySquare(vector<Feature>& trainingSet){
	vector<vector<Feature> > splitBySquares = vector<vector<Feature> >(nMLPs);
			
	for(unsigned int i = 0;i < trainingSet.size();i++){
		splitBySquares[trainingSet[i].getSquareId()].push_back(trainingSet[i]);	
	}
	return splitBySquares;
}

//-------------------end: data creation methods---------------
//-------------------start: bottomlevel-----------------------
void MLPController::createDataBottomLevel(vector<vector<Feature> >& splitTrain, vector<vector<Feature> >& splitVal){
	vector<Feature> trainingSet;
 	vector<Feature> validationSet;
	
	splitTrain = splitUpDataBySquare(createRandomFeatureVector(trainingSet));
	splitVal   = splitUpDataBySquare(createCompletePictureSet(validationSet,trainSize,trainSize+validationSize));
	
	for(int i=0;i<nMLPs;i++){
		setMinAndMaxValueNorm(splitTrain[i],0);
	}
	for(int i=0;i<nMLPs;i++){
		numPatchesPerSquare.push_back(splitVal[i].size()/validationSize);
		std::cout<< "numPatchersPerSquare[" << i << "]: " << numPatchesPerSquare[i] << std::endl;
	}
	
	trainingSet.clear();
	validationSet.clear();
}

vector<Feature>& MLPController::createRandomFeatureVector(vector<Feature>& trainingData){
    unsigned int nPatches = settings.scannerSettings.nRandomPatches;
   
	vector<Feature> testData;
	std::cout << "create random feature vector of size: " << nPatches << std::endl;
	
	for(size_t pIdx = 0; pIdx < nPatches; ++pIdx){
		Patch patch = imageScanner.getRandomPatch(dataset.getTrainImagePtr(rand() % trainSize));
		Feature newFeat = featExtr.extract(patch);
		newFeat.setSquareId(calculateSquareOfPatch(patch));
		trainingData.push_back(newFeat);    
	}
	return trainingData;	
}
//--------------------end: bottom level---------------------------
//--------------------start: first level--------------------------
void MLPController::setFirstLevelData(vector<vector<Feature> >& splitDataBottom,vector<Feature>& dataFirstLevel, int sizeData){
		for(int i = 0; i < sizeData;i++){			
		vector<double> input;
		input.reserve(nHiddenBottomLevel*nMLPs);
		
		for(int j=0;j<nMLPs;j++){			
			vector<Feature>::const_iterator first = splitDataBottom[j].begin()+(numPatchesPerSquare[j]*i);
			vector<Feature>::const_iterator last = splitDataBottom[j].begin()+(numPatchesPerSquare[j]*(i+1));

			vector<double> inputTemp = vector<double>(nHiddenBottomLevel,-10.0);
			
			mlps[0][j].returnHiddenActivation(vector<Feature>(first,last),inputTemp);
			input.insert(input.end(),inputTemp.begin(),inputTemp.end());
		}
		
		Feature newFeat = new Feature(input);	
		newFeat.setLabelId(splitDataBottom[0][i*numPatchesPerSquare[0]].getLabelId());
		
		dataFirstLevel.push_back(newFeat);
	}
}
/* In the first level the mlp training is now based on complete images. 
 * This is different from the bottom level where it is based on random patches */
void MLPController::createDataFirstLevel(vector<Feature>& inputTrainFirstLevel, vector<Feature>& inputValFirstLevel){
  	vector<Feature> trainingSet;
	vector<Feature> validationSet;
	
	//increasing the stride has to happens somewhere here to decrease the number of pictures in the training set
	
	vector<vector<Feature> > splitTrain = splitUpDataBySquare(createCompletePictureSet(trainingSet,0,trainSize));
	vector<vector<Feature> > splitVal   = splitUpDataBySquare(createCompletePictureSet(validationSet,trainSize,trainSize+validationSize));

	trainingSet.clear();
	validationSet.clear();
	
	for(int i=0;i<nMLPs;i++){
		normalizeInput(splitTrain[i],0);
		normalizeInput(splitVal[i],0);
	}
	
	setFirstLevelData(splitTrain,inputTrainFirstLevel,trainSize); 	//set training set
	setFirstLevelData(splitVal,inputValFirstLevel,validationSize); 	//set validation set
    

	setMinAndMaxValueNorm(inputTrainFirstLevel,1);    //set min and max value for first level normalization
}
//-----------start: training MLP's-------------------------
void MLPController::trainMLP(MLPerceptron& mlp,vector<Feature>& trainingSet, vector<Feature>& validationSet, int numPatchesPerSquare){
    mlp.train(trainingSet,validationSet,numPatchesPerSquare);
}

void MLPController::trainMutipleMLPs(){
	vector<vector<Feature> > splitTrain;
	vector<vector<Feature> > splitVal;
	
	createDataBottomLevel(splitTrain,splitVal);
	
	for(int i=0;i<nMLPs;i++){
		normalizeInput(splitTrain[i],0);
		normalizeInput(splitVal[i],0);
	}
	
	for(int i=0;i<nMLPs;i++){ 		
		trainMLP(mlps[0][i],splitTrain[i],splitVal[i],numPatchesPerSquare[i]);
		std::cout << "mlp["<<i<<"] from level 0 finished training" << std::endl << std::endl;
    }
    
    std::cout << "create training data for first level... " << std::endl;
    
    vector<Feature> inputTrainFirstLevel;
	vector<Feature> inputValFirstLevel;
    
    createDataFirstLevel(inputTrainFirstLevel,inputValFirstLevel);
    
    std::cout << "min value first level: " << minValues[1] << std::endl;
    std::cout << "max value first level: " << maxValues[1] << std::endl;
    
    normalizeInput(inputTrainFirstLevel,1);
    normalizeInput(inputValFirstLevel,1);
    
    for(unsigned int i = 0; i < inputTrainFirstLevel.size();i++){
		std::cout << "feature ["<<i<<"]: " << inputTrainFirstLevel[i].getLabelId() << std::endl;
		for(int j = 0; j < inputTrainFirstLevel[i].size;j++){
			std::cout << inputTrainFirstLevel[i].content[j] << ", ";
		}
		std::cout << std::endl << std::endl;
	}
    
    int totalPatches = 0;
    for(int i=0;i<nMLPs;i++){
		totalPatches += numPatchesPerSquare[i];
	}
    
	trainMLP(mlps[1][0],inputTrainFirstLevel,inputValFirstLevel,totalPatches);
	std::cout << "mlp[0] from level 1 finished training" << std::endl;
}
//--------------end: training MLP's-----------------------------
//-------------start: MLP classification------------------------
unsigned int MLPController::mlpMultipleClassify(Image* im){
	int numOfImages = 1;
	
	vector<Patch> patches;
	vector<Feature> dataFeatures;

	//extract patches
	patches = imageScanner.scanImage(im);

	//allocate for new features
	dataFeatures.reserve(patches.size());
      
	//extract features from all patches
	for(size_t patch = 0; patch < patches.size(); ++patch){
		Feature newFeat = featExtr.extract(patches[patch]);
		newFeat.setSquareId(patches[patch].getSquare());
		dataFeatures.push_back(newFeat);
	}
	
	vector<vector<Feature> > testFeaturesBySquare = splitUpDataBySquare(normalizeInput(dataFeatures,0)); //split test features by square
	
	vector<Feature> testDataFirstLevel; //empty feature vector that will be filled with first level features
	setFirstLevelData(testFeaturesBySquare,testDataFirstLevel,numOfImages);
	
	//for(int i=0;i<testDataFirstLevel[0].size;i++){
	//	std::cout << testDataFirstLevel[0].content[i] << ", "; 
	//}
	//std::cout << std::endl;
	
	normalizeInput(testDataFirstLevel,1); 
	
	vector<double> votingHisto = mlps[1][0].classifyPooling(testDataFirstLevel);
	
	//for(int i=0;i<10;i++){
	//	std::cout << votingHisto[i] << std::endl;
	//}
	std::cout << std::endl;
	//std::cout << "answer: " << answer << std::endl;
	int answer = 0;
	return answer;
}
//---------------------end:MLP classification------------------------
