#include <csvm/csvm_classifier.h>
//
using namespace std;
using namespace csvm;

CSVMClassifier::CSVMClassifier(){
   srand(time(NULL));
   
}

void CSVMClassifier::initSVMs(){
   double learningRate = 0.05;
   svms.reserve(codebook.getNClasses());
   for(size_t svmIdx = 0; svmIdx < codebook.getNClasses(); ++svmIdx){
      svms.push_back(SVM(dataset.getSize(), codebook.getNClasses(), codebook.getNCentroids(), learningRate, svmIdx, codebook.getNCentroids()));
   }
}


void CSVMClassifier::setSettings(string settingsFile){
   settings.readSettingsFile(settingsFile);
   analyser.setSettings(settings.analyserSettings);
   imageScanner.setSettings(settings.scannerSettings);
}


void CSVMClassifier::exportCodebook(string filename){
   codebook.exportCodebook(filename);
}

void CSVMClassifier::importCodebook(string filename){
   codebook.importCodebook(filename);
}

//DEBUG


void checkEqualPatches(vector<Patch> patches){
   unsigned int nEquals = 0;
   unsigned int nPatches = patches.size();
   
   for(size_t pIdx = 0 ; pIdx < nPatches - 1; ++ pIdx){
      for(size_t pIdx1 = pIdx + 1; pIdx1 < nPatches; ++ pIdx1){
         if(patches[pIdx].equals(patches[pIdx1]))
            ++nEquals;
      }
   }
   cout << "There are " << nEquals << " equal patches\n";
}

void CSVMClassifier::constructCodebook(){
   unsigned int nPatches = 10;  //number of random patches from each image
   unsigned int nClasses = dataset.getNumberClasses();
   

   
   vector<Patch> patches;
   vector<Feature> features;
   
   features.reserve(nPatches);
   
   pretrainDump.clear();
   pretrainDump.resize(nClasses);
   unsigned int nImages = dataset.getSize();
   cout << "constructing codebooks for " << nClasses << " classes using " << nImages << " images in total\n";
   for(size_t cl = 0; cl < nClasses; ++cl){
      
      //cout << "parsing class " << cl << endl;
      nImages = dataset.getNumberImagesInClass(cl);
      //cout << nImages << " images\n";
      
      pretrainDump[cl].reserve(nPatches * nImages);
      //cout << "space reserved\n";
      cout << "Scanning " << nImages << " images\n";
      for(size_t im = 0; im < nImages; ++im){
         //cout << "scanning patches\n";
         patches = imageScanner.getRandomPatches(dataset.getImagePtr(im), nPatches, 8, 8);
         //checkEqualPatches(patches);
         features.clear();
         features.reserve(nPatches);
         //cout << "extracting patches..\n";
         for(size_t patchIdx = 0; patchIdx < nPatches; ++patchIdx){
            Feature newFeat = featExtr.extract(patches[patchIdx]);
            features.push_back(newFeat);
            //cout << "first element from new Feature = " << newFeat.content[0] << ", which should equal " << features[patchIdx].content[0] << endl;
         }
         //checkEqualFeatures(features);
         pretrainDump[cl].insert(pretrainDump[cl].end(),features.begin(),features.end());
         
      }
      //checkEqualFeatures(pretrainDump[cl]);
      //cout << "end feature meditation\n";
      //cout << pretrainDump[cl].size() << " features extracted for class " << cl << "\n";
      //checkEqualFeatures(pretrainDump[cl]);
      codebook.constructCodebook(pretrainDump[cl],cl);
      //checkEqualFeatures(pretrainDump[cl]);
      cout << "done constructing codebook for class " << cl << " using " << nImages << " images, " << nPatches << " patches each: " << nPatches * nImages<<" in total.\n";
   }
   pretrainDump.clear();
}

void CSVMClassifier::trainSVMs(){
   unsigned int datasetSize = dataset.getSize();
   vector < vector < Feature > > datasetActivations;
   vector < Feature > dataFeatures;
   vector < Patch > patches;
   
   //allocate space for more vectors
   datasetActivations.reserve(datasetSize);
   cout << "collecting activations for trainingsdata..\n";
   //for all trainings imagages:
   for(size_t dataIdx = 0; dataIdx < datasetSize; ++dataIdx){
      
      //extract patches
      patches = imageScanner.scanImage(dataset.getImagePtr(dataIdx), 8, 8, 1, 1);
      
      //clear previous features
      dataFeatures.clear();
      //allocate for new
      dataFeatures.reserve(patches.size());
      
      //extract features from all patches
      for(size_t patch = 0; patch < patches.size(); ++patch)
         dataFeatures.push_back(featExtr.extract(patches[patch]));
      patches.clear();
      
      //Feature* f = &dataFeatures[0];
      
      //cout << "datacontent: " << f->size << endl;
      //get cluster activations for the features
      //cout << "pushing back " << codebook.getActivations(dataFeatures)[0].content.size() << "act feats\n";
      datasetActivations.push_back(codebook.getActivations(dataFeatures));
      
      for(size_t pIdx = 0; pIdx < datasetActivations[dataIdx].size(); ++pIdx){
         for(size_t centr = 0; centr < datasetActivations[dataIdx][pIdx].content.size(); ++centr)
            ;//cout << "activation image " << dataIdx << " from class " << pIdx << " at centroid " << centr << " = " << datasetActivations[dataIdx][pIdx].content[centr] << endl;
      }
   }
   cout << "I can get activations me!\n";
   //train the SVMs with the gained activations
   for(size_t svmIdx = 0; svmIdx < svms.size(); ++svmIdx){
      svms[svmIdx].train(datasetActivations);
   }
}

unsigned int CSVMClassifier::classify(Image* image){
   unsigned int nClasses = codebook.getNClasses();
   cout << "nClasses = " << nClasses << endl;
   vector<Patch> patches;
   vector<Feature> dataFeatures;
   //extract patches
   patches = imageScanner.scanImage(image, 8, 8, 1, 1);
   

   //allocate for new
   dataFeatures.reserve(patches.size());
   
   //extract features from all patches
   for(size_t patch = 0; patch < patches.size(); ++patch)
      dataFeatures.push_back(featExtr.extract(patches[patch]));
   
   patches.clear();

   
   vector<double> results(nClasses, 0);
   double maxResult = -99999;
   unsigned int maxLabel=0;
   for(size_t cl = 0; cl < nClasses; ++cl){
      results[cl] = svms[cl].classify(codebook.getActivations(dataFeatures), &codebook);
      
      cout << "SVM " << cl << " says " << results[cl] << endl;  
      if(results[cl] > maxResult){
         maxResult = results[cl];

         maxLabel = cl;
      }
   }
   return maxLabel;
}