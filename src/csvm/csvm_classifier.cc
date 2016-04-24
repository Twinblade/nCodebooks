#include <csvm/csvm_classifier.h>
#include <iomanip>

/* This class implements the general pipeline of the system.
 * Based on the settingsfile, it will construct codebooks, describe images and train/test the system.
 * 
 * 
 * 
 * 
 */

using namespace std;
using namespace csvm;

vector<MLPerceptron> MLPs;

//initialize random
CSVMClassifier::CSVMClassifier(){
   srand(time(NULL)); 
   deepCodebook = new DeepCodebook(&featExtr, &imageScanner, &dataset);
}

CSVMClassifier::~CSVMClassifier(){
   delete deepCodebook;
}

bool CSVMClassifier::getGenerateCB(){
   return codebook.getGenerate();
}

//initialize the SVMs, by telling them the dataset size, amount of classes, centroids, and the respective label of the SVM
void CSVMClassifier::initSVMs(){
   
   unsigned int ensembleSize = dataset.getNumberClasses();
   
   svms.reserve(dataset.getNumberClasses());
   for(size_t svmIdx = 0; svmIdx < ensembleSize; ++svmIdx){
      svms.push_back(SVM(dataset.getTrainSize(), codebook.getNClasses(), codebook.getNCentroids(), svmIdx));
      svms[svmIdx].setSettings(settings.svmSettings);
      svms[svmIdx].debugOut = settings.debugOut;
      svms[svmIdx].normalOut = settings.normalOut;
   }
   
}

//read settings file, and pass the settings to respective modules
void CSVMClassifier::setSettings(string settingsFile){
   settings.readSettingsFile(settingsFile);
   //analyser.setSettings(settings.analyserSettings);
   imageScanner.setSettings(settings.scannerSettings);
   imageScanner.debugOut = settings.debugOut;
   imageScanner.normalOut = settings.normalOut;
   
   dataset.setSettings(settings.datasetSettings);
   dataset.debugOut = settings.debugOut;
   dataset.normalOut = settings.normalOut;
   
   settings.codebookSettings.kmeansSettings.normalOut = normalOut;
   codebook.setSettings(settings.codebookSettings);
   codebook.debugOut = settings.debugOut;
   codebook.normalOut = settings.normalOut;
   
   
   
   featExtr.setSettings(settings.featureSettings);
   featExtr.debugOut = settings.debugOut;
   featExtr.normalOut = settings.normalOut;
   
   settings.netSettings.nCentroids = settings.codebookSettings.numberVisualWords;
   
   linNetwork.setSettings(settings.netSettings);
   linNetwork.debugOut = settings.debugOut;
   linNetwork.normalOut = settings.normalOut;
   
   convSVM.setSettings(settings.convSVMSettings);
   convSVM.debugOut = settings.debugOut;
   convSVM.normalOut = settings.normalOut;
   
   //mlp.setSettings(settings.mlpSettings);
   initMLPs();
}

//Train the system
void CSVMClassifier::train(){
   switch(settings.classifier){
      case CL_SVM:
         if(normalOut) cout << "Training SVM..\n";
         trainClassicSVMs();
         
         break;
      case CL_CSVM:
         if(normalOut) cout << "Training Conv SVM..\n";
         trainConvSVMs();
         
         break;
      case CL_LINNET:
         if(normalOut) cout << "Training LinNet..\n";
         trainLinearNetwork();
		case CL_MLP:
			if(normalOut) cout << "Training MLP..\n";
			trainMutipleMLPs();
			//trainMLP();
			
         break;
      default:
         cout << "WARNING! couldnt recognize selected classifier!\n";
   }
}

//Classify an image, given its pointer
unsigned int CSVMClassifier::classify(Image* im){
   unsigned int result = 0;
   
   switch(settings.classifier){
      case CL_SVM:
         result = classifyClassicSVMs(im, false); //return value should be processed
         break;
      case CL_CSVM:
         result = classifyConvSVM(im);
         break;
      case CL_LINNET:
         result = lnClassify(im);
         break;
		case CL_MLP:
			//result = mlpMultipleClassify(im);
			result = mlpClassify(im);
			break;
   }
   return result;
}


//export the current codebook to file (Only works for the normal codebook, not yet for the deep bow)
void CSVMClassifier::exportCodebook(string filename){
   codebook.exportCodebook(filename);
}


//import the current codebook
void CSVMClassifier::importCodebook(string filename){
   codebook.importCodebook(filename);
}


void CSVMClassifier::constructDeepCodebook(){
   deepCodebook->setSettings(settings.dcbSettings);
   deepCodebook->debugOut = settings.debugOut;
   deepCodebook->normalOut = settings.normalOut;
   deepCodebook->generateCentroids();
   if(normalOut) cout << "Done constructing deep codebook\n";
}


//return number of classes
unsigned int CSVMClassifier::getNoClasses(){
   return dataset.getNumberClasses();
}

void CSVMClassifier::initMLPs(){
	mlps.reserve(settings.mlpSettings.nMLPs);
	for(int i = 0; i < settings.mlpSettings.nMLPs;i++){
		MLPerceptron mlp;
		std::cout << "mlp["<<i<<"]:";
		mlp.setSettings(settings.mlpSettings); 
		mlps.push_back(mlp);
	}
}

vector<Feature>& CSVMClassifier::createValidationSet(vector<Feature>& validationSet){
	int amountOfImagesCrossVal = dataset.getTrainSize() * settings.mlpSettings.crossValidationSize;
	int noPatchesPerSquare  = validationSet.size() / amountOfImagesCrossVal; //Needs a fix, is now 0
	
	vector<Patch> patches;   
  
    validationSet.reserve(noPatchesPerSquare*amountOfImagesCrossVal);
  
	for(int i = dataset.getTrainSize() - amountOfImagesCrossVal; i < dataset.getTrainSize();i++){
		Image* im = dataset.getTrainImagePtr(i);
		 
		//extract patches
		patches = imageScanner.scanImage(im);
      
		//extract features from all patches
		for(size_t patch = 0; patch < patches.size(); ++patch){
			Feature newFeat = featExtr.extract(patches[patch]);
			newFeat.setSquareId(patches[patch].getSquare());
			validationSet.push_back(newFeat);
		}
	}
	return validationSet;
}

vector<Feature>& CSVMClassifier::createRandomFeatureVector(vector<Feature>& trainingData){
    unsigned int nPatches = settings.scannerSettings.nRandomPatches;
    int sizeTrainingSet = (int)(dataset.getTrainSize()*(1-settings.mlpSettings.crossValidationSize));
    
	vector<Feature> testData;
  
	std::cout << "Feature extraction training set..." << std::endl;
   for(size_t pIdx = 0; pIdx < nPatches; ++pIdx){
      Patch patch = imageScanner.getRandomPatch(dataset.getTrainImagePtr(rand() % sizeTrainingSet));
      //std::cout << "(classifier) getSquare: " << patch.getSquare() << std::endl;
      Feature newFeat = featExtr.extract(patch);
      newFeat.setSquareId(patch.getSquare());
      trainingData.push_back(newFeat);    
   }
	return trainingData;	
}
void CSVMClassifier::trainMLP(MLPerceptron& mlp,vector<Feature>& trainingSet, vector<Feature>& validationSet){
    double crossvalidationSize = dataset.getTrainSize() * settings.mlpSettings.crossValidationSize;
    int noPatchesPerSquare = validationSet.size()/crossvalidationSize; 
    mlp.train(trainingSet,validationSet,noPatchesPerSquare);
}

//void CSVMClassifier::trainMLP(){
//    int noPatchesPerImage = imageScanner.scanImage(dataset.getTrainImagePtr(0)).size(); 
    
//    vector<Feature> trainingSet;
//    vector<Feature> validationSet;
    
//   mlp.train(createRandomFeatureVector(trainingSet),createValidationSet(validationSet),noPatchesPerImage);
//}

void CSVMClassifier::trainMutipleMLPs(){
	//int noPatchesPerImage = imageScanner.scanImage(dataset.getTrainImagePtr(0)).size(); 
	//double crossvalidationSize = dataset.getTrainSize() * settings.mlpSettings.crossValidationSize;
 	
 	vector<Feature> trainingSet;
 	vector<Feature> validationSet;

	vector<vector<Feature> > splitTrain = splitUpDataBySquare(createRandomFeatureVector(trainingSet));
	vector<vector<Feature> > splitVal   = splitUpDataBySquare(createValidationSet(validationSet));
	
	trainingSet.clear();
	validationSet.clear();
	
	std::cout << "(classifier) numPatches top left: " << splitTrain[0].size()  << std::endl; 
	std::cout << "(classifier) numPatches top right: " << splitTrain[1].size()  << std::endl;
	std::cout << "(classifier) numPatches bottom left: " << splitTrain[2].size()  << std::endl;
	std::cout << "(classifier) numPatches bottom right: " << splitTrain[3].size()  << std::endl; 

	//vector<vector<Feature> > splitVal = splitUpValSet(createValidationSet(validationSet));
	
	for(unsigned int i=0;i<mlps.size();i++){ 		
		trainMLP(mlps[i],splitTrain[i],splitVal[i]);
		std::cout << "(classifier) trained mlp["<<i<<"]" << std::endl;
    }
    splitTrain.clear();
    splitVal.clear();
    
    exit(-1);	
}

vector<vector<Feature> > CSVMClassifier::splitUpDataBySquare(vector<Feature>& trainingSet){
	vector<vector<Feature> > splitBySquares = vector<vector<Feature> >(settings.mlpSettings.nMLPs);
	
	std::cout << "(classifier) trainingSet.size: "<< trainingSet.size() << std::endl;
		
	for(unsigned int i = 0;i < trainingSet.size();i++){
		splitBySquares[trainingSet[i].getSquareId()].push_back(trainingSet[i]);	
	}
	return splitBySquares;
}

//vector<vector<Feature> > CSVMClassifier::splitUpValSet(vector<Feature> validationSet){
//	vector<vector<Feature> > splitBySquares = vector<vector<Feature> >(settings.mlpSettings.nMLPs);;
//	std::cout << "(classifier) validationSet.size: "<< validationSet.size() << std::endl;
		
//	for(unsigned int i = 0;i < trainingSet.size();i++){
//		splitBySquares[trainingSet[i].getSquareId()].push_back(trainingSet[i]);	
//	}
//	return splitBySquares;

//}


unsigned int CSVMClassifier::mlpClassify(Image* im){
	
	vector<Patch> patches;
	vector<Feature> dataFeatures;
      
	//extract patches
    patches = imageScanner.scanImage(im);

    //allocate for new features
    dataFeatures.reserve(patches.size());
      
    //extract features from all patches
    for(size_t patch = 0; patch < patches.size(); ++patch)
		dataFeatures.push_back(featExtr.extract(patches[patch]));
		
	return mlps[0].classify(dataFeatures);
}

//unsigned int CSVMClassifier::mlpMultipleClassify(Image* im){
//  vector<Patch> patches;
//	vector<Feature> dataFeatures;
//	vector<int> mostProbableClass = vector<int>(settings.mlpSettings.nOutputUnits,0);;      

	//extract patches
//  patches = imageScanner.scanImage(im);

    //allocate for new features
//  dataFeatures.reserve(patches.size());
      
    //extract features from all patches
//  for(size_t patch = 0; patch < patches.size(); ++patch)
//		dataFeatures.push_back(featExtr.extract(patches[patch]));
		
//  vector<vector<Feature>> testFeatures = splitTrainSet(dataFeatures);

//  for(int i=0;i<MLPs.size();i++)
//		mostProbableClass[MLPs[i].classify(testFeatures[i])] += 1;
// 	return (unsigned int) *std::max_element(mostProbableClass.begin(), mostProbableClass.end());
//}

//construct a codebook using the current dataset
void CSVMClassifier::constructCodebook(){
   cout << "hoi\n";
   if(settings.codebook == CB_DEEPCODEBOOK){
      constructDeepCodebook();
      return;
   }else if(settings.codebook == CB_MLP){
      //cout << "should be training the mlp...\n";
      //trainMLP();
      return;
   } 
      
   //unsigned int nClasses = dataset.getNumberClasses();
   
   unsigned int nPatches = settings.scannerSettings.nRandomPatches;
   
   vector<Feature> pretrainDump;


   for(size_t pIdx = 0; pIdx < nPatches; ++pIdx){
      
      //patches = imageScanner.getRandomPatches(dataset.getImagePtrFromClass(im, cl));
      Patch patch = imageScanner.getRandomPatch(dataset.getImagePtr(rand() % dataset.getTotalImages()));
      Feature newFeat = featExtr.extract(patch);
      pretrainDump.push_back(newFeat);//insert(pretrainDump[cl].end(),features.begin(),features.end());

      
   }

   if(debugOut) cout << "Collected " << pretrainDump.size()<< " features\n";
   codebook.constructCodebook(pretrainDump);
   
   if(debugOut) cout << "done constructing codebook using "  << settings.scannerSettings.nRandomPatches << " patches\n";
   
   pretrainDump.clear();
}


void CSVMClassifier::trainConvSVMs(){
   unsigned int nTrainImages = dataset.getTrainSize();
   vector < vector < double > > datasetActivations;
   vector < Feature > dataFeatures;
   vector < Patch > patches;
   
   //allocate space for more vectors
   datasetActivations.reserve(nTrainImages);
   //for all trainings imagages:
   for(size_t dataIdx = 0; dataIdx < nTrainImages; ++dataIdx){
      vector<double> dataActivation;
      //extract patches
      
      if(settings.codebook == CB_CODEBOOK){
         
         patches = imageScanner.scanImage(dataset.getTrainImagePtr(dataIdx));
         dataActivation.clear();
         dataFeatures.clear();
         //clear previous features
         
         //allocate for new features
         dataFeatures.reserve(patches.size());
         
         //extract features from all patches
         for(size_t patch = 0; patch < patches.size(); ++patch){
            dataFeatures.push_back(featExtr.extract(patches[patch]));
         }
         
         patches.clear();
         
         //get cluster activations for the features
         dataActivation = codebook.getActivations(dataFeatures); 
         dataFeatures.clear();
      
         patches.clear();
         

         //get cluster activations for the features
         datasetActivations.push_back(dataActivation);
         dataActivation.clear();
     }else if(settings.codebook == CB_DEEPCODEBOOK){
        datasetActivations.push_back(deepCodebook->getActivations(dataset.getTrainImagePtr(dataIdx)));
     }else if(settings.codebook == CB_MLP){
        /*
        patches = imageScanner.scanImage(dataset.getTrainImagePtr(dataIdx));
         dataActivation.clear();
         dataFeatures.clear();
         //clear previous features
         
         //allocate for new features
         dataFeatures.reserve(patches.size());
         
         //extract features from all patches
         for(size_t patch = 0; patch < patches.size(); ++patch){
            dataFeatures.push_back(featExtr.extract(patches[patch]));
         */}
         
         patches.clear();
          
         dataFeatures.clear();
      
         patches.clear();
         

         //get cluster activations for the features
         datasetActivations.push_back(dataActivation);
         dataActivation.clear();
   }
   convSVM.train(datasetActivations, &dataset);
}


unsigned int CSVMClassifier::classifyConvSVM(Image* image){
   vector<Patch> patches;
   vector<Feature> dataFeatures;
   vector<double> dataActivation;
   
   if(settings.codebook == CB_CODEBOOK){
      
      vector<Patch> patches;
      vector<Feature> dataFeatures;
      
      //extract patches
      patches = imageScanner.scanImage(image);
         
      //clear previous features
      dataFeatures.clear();
      //allocate for new features
      dataFeatures.reserve(patches.size());
      
      //extract features from all patches
      for(size_t patch = 0; patch < patches.size(); ++patch)
         dataFeatures.push_back(featExtr.extract(patches[patch]));
      
      patches.clear();
      dataActivation = codebook.getActivations(dataFeatures); 
   }else if(settings.codebook == CB_DEEPCODEBOOK){      
      dataActivation = deepCodebook->getActivations(image);
   }else if(settings.codebook == CB_MLP){
      vector<Patch> patches;
      vector<Feature> dataFeatures;
      
      //extract patches
      patches = imageScanner.scanImage(image);
         
      //clear previous features
      dataFeatures.clear();
      //allocate for new features
      dataFeatures.reserve(patches.size());
      
      //extract features from all patches
      for(size_t patch = 0; patch < patches.size(); ++patch)
         dataFeatures.push_back(featExtr.extract(patches[patch]));
      
      patches.clear();
      cout << "class: train mlp\n";  
   }

   return convSVM.classify(dataActivation);
}

bool CSVMClassifier::useOutput(){
   return settings.normalOut;

}

//train the regular-SVM
void CSVMClassifier::trainClassicSVMs(){
   unsigned int nTrainImages = dataset.getTrainSize();
   unsigned int nClasses = dataset.getNumberClasses(); 
   unsigned int nCentroids; 
   
   vector < vector<double> > datasetActivations;
   vector < Feature > dataFeatures;
   vector < Patch > patches;
   vector < vector<double> > dataKernel(nTrainImages);
   vector<double> dataActivation;

   //allocate space for more vectors
   datasetActivations.reserve(nTrainImages);
   //for all trainings imagages:
   for(size_t dataIdx = 0; dataIdx < nTrainImages; ++dataIdx){
      
      //extract patches
      if(settings.codebook == CB_CODEBOOK){
         patches = imageScanner.scanImage(dataset.getTrainImagePtr(dataIdx));
         dataActivation.clear();
         dataFeatures.clear();
         //clear previous features
         
         //allocate for new features
         dataFeatures.reserve(patches.size());
         
         //extract features from all patches
         for(size_t patch = 0; patch < patches.size(); ++patch){
            dataFeatures.push_back(featExtr.extract(patches[patch]));
            //cout << "Patch at " << patches[patch].getX() << ", " << patches[patch].getY() << endl;
         }
         
         
         patches.clear();
         
         //get cluster activations for the features
         dataActivation = codebook.getActivations(dataFeatures); 
         dataFeatures.clear();
      
         patches.clear();
         

         //get cluster activations for the features
         datasetActivations.push_back(dataActivation);
         dataActivation.clear();
     }else{
        datasetActivations.push_back(deepCodebook->getActivations(dataset.getTrainImagePtr(dataIdx)));
     }
   }
   nClasses = dataset.getNumberClasses();
   nCentroids = datasetActivations[0].size();
   
   if(debugOut) cout << "nClasses = " << nClasses << endl;
   if(debugOut) cout << "nCentroids = " << nCentroids << endl;
   
   if(debugOut) cout << "Calculating similarities\n";
   //calculate similarity kernal between activation vectors
   for(size_t dIdx0 = 0; dIdx0 < nTrainImages; ++dIdx0){
      //cout << "done with similarity of " << dIdx0 << endl;
      for(size_t dIdx1 = 0; dIdx1 <= dIdx0; ++dIdx1){
         dataKernel[dIdx0].resize(dIdx0 + 1);
         double sum = 0;
	 
         if(settings.svmSettings.kernelType == RBF){
            


            for(size_t centr = 0; centr < nCentroids; ++centr){
               sum += (datasetActivations[dIdx0][centr] - datasetActivations[dIdx1][centr])*(datasetActivations[dIdx0][centr] - datasetActivations[dIdx1][centr]);
            }
            
            dataKernel[dIdx0][dIdx1] = exp((-1.0 * sum)/settings.svmSettings.sigmaClassicSimilarity);
            //dataKernel[dIdx1][dIdx0] = dataKernel[dIdx0][dIdx1];
            
         }else if (settings.svmSettings.kernelType == LINEAR){

            for(size_t cl = 0; cl < 1; ++cl){

               for(size_t centr = 0; centr < nCentroids; ++centr){
                  sum += (datasetActivations[dIdx0][centr] * datasetActivations[dIdx1][centr]);
               }
            }
            //cout << "Writing " << sum << " to " << dIdx0 << ", " << dIdx1 << endl;
            dataKernel[dIdx0][dIdx1] = sum;
            //dataKernel[dIdx1][dIdx0] = sum;
         }else
            cout << "CSVM::svm::Error! No valid kernel type selected! Try: RBF or LINEAR\n"  ;
         
      }
   }

   
   for(size_t cl = 0; cl < nClasses; ++cl){
      svms[cl].trainClassic(dataKernel, &dataset);  
   }
   classicTrainActivations = datasetActivations;
}


//classify an image using the regular-SVM
unsigned int CSVMClassifier::classifyClassicSVMs(Image* image, bool printResults){
   unsigned int nClasses = dataset.getNumberClasses();
   
   vector<double> dataActivation;
   if(settings.codebook == CB_CODEBOOK){
      vector<Patch> patches;
      vector<Feature> dataFeatures;
      
      //extract patches
      patches = imageScanner.scanImage(image);
         
      //clear previous features
      dataFeatures.clear();
      //allocate for new features
      dataFeatures.reserve(patches.size());
      
      //extract features from all patches
      for(size_t patch = 0; patch < patches.size(); ++patch)
         dataFeatures.push_back(featExtr.extract(patches[patch]));
      patches.clear();
      dataActivation = codebook.getActivations(dataFeatures); 
   }else{
      
      
      dataActivation = deepCodebook->getActivations(image);
   }
   //append centroid activations to activations from 0th quadrant.
   nClasses = dataset.getNumberClasses();   //normalize
   

   //reserve space for results
   vector<double> results(nClasses, 0);
   
   double maxResult = -99999;
   unsigned int maxLabel=0;
   //get max-result label
   for(size_t cl = 0; cl < nClasses; ++cl){
      results[cl] = svms[cl].classifyClassic(dataActivation, classicTrainActivations, &dataset);
      
      if(printResults)
         cout << "SVM " << cl << " says " << results[cl] << endl;  
      if(results[cl] > maxResult){
         maxResult = results[cl];

         maxLabel = cl;
      }
   }
   return maxLabel;
}



void CSVMClassifier::trainLinearNetwork(){
   unsigned int nTrainImages = dataset.getTrainSize();
   vector < vector < double > > datasetActivations;
   
   vector < Patch > patches;
   vector<double> dataActivation;
   //allocate space for more vectors
   datasetActivations.reserve(nTrainImages);
   //for all trainings imagages:
   for(size_t dataIdx = 0; dataIdx < nTrainImages; ++dataIdx){
      vector < Feature > dataFeatures;
      //extract patches
      if(settings.codebook == CB_CODEBOOK){
         patches = imageScanner.scanImage(dataset.getTrainImagePtr(dataIdx));
         dataActivation.clear();
         dataFeatures.clear();
         //clear previous features
         
         //allocate for new features
         dataFeatures.reserve(patches.size());
         
         //cout << patches[qIdx].size() << " patches" << endl;
         //extract features from all patches
         for(size_t patch = 0; patch < patches.size(); ++patch){
            dataFeatures.push_back(featExtr.extract(patches[patch]));
         }
         
         patches.clear();
         
         //get cluster activations for the features
         dataActivation = codebook.getActivations(dataFeatures); 
         dataFeatures.clear();
      
         patches.clear();
         

         //get cluster activations for the features
         datasetActivations.push_back(dataActivation);
         dataActivation.clear();
     }else{
        datasetActivations.push_back(deepCodebook->getActivations(dataset.getTrainImagePtr(dataIdx)));
     }
   }
   //train the Linear Netwok with the gained activations
   linNetwork.train(datasetActivations, &dataset);
}



unsigned int CSVMClassifier::lnClassify(Image* image){
   vector<Patch> patches;
   vector<Feature> dataFeatures;
   vector<double> dataActivation;
   //extract patches
  
   if(settings.codebook == CB_CODEBOOK){
      vector<Patch> patches;
      vector<Feature> dataFeatures;
      
      //extract patches
      patches = imageScanner.scanImage(image);
         
      //clear previous features
      dataFeatures.clear();
      //allocate for new features
      dataFeatures.reserve(patches.size());
      
      //extract features from all patches
      for(size_t patch = 0; patch < patches.size(); ++patch)
         dataFeatures.push_back(featExtr.extract(patches[patch]));
      patches.clear();
      dataActivation = codebook.getActivations(dataFeatures); 
   }else{
      
      
      dataActivation = deepCodebook->getActivations(image);
   }
   

   return linNetwork.classify(dataActivation);

}
