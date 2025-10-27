#include "Init.h"

uint8_t cmdFlags = 0;
std::string fileIn = "";
std::string fileOut = "";
std::string fileDataProcessed = "";

int minsup = -1;

std::vector<std::vector<std::string>> dataArr;
std::map<std::vector<std::string>, int> supTable;
std::unordered_map<std::string, int> cacheSingles;

std::vector<Row> patOut;



/*
Returns -1 on failure, 0 on success.
*/
int main(int argc, char* argv[]){
	//Parse inputs
	try {parseInputs(argc, argv);}
	catch (const std::invalid_argument& e) {
        fprintf(stderr, "Usage error: %s\n", e.what());
        return -1;
    }

    //Open and read data, build supTable for 1-itemsets, and sort observations by sup count
    try {readData();}
    catch (const std::invalid_argument& e) {
        perror(e.what());
        return -1;
    }

    //Perform apriori algorithm
    try {apriori();}
    catch (const std::invalid_argument& e) {
        perror(e.what());
        return -1;
    }

    if(cmdFlags & _NUMERIC_FLAG){
	    try {printResNumeric();}
	    catch (const std::invalid_argument& e) {
    		printf("here");
	        perror(e.what());
	        return -1;
	    }
    }else{
	    //Write supTable to file (lexicographical sorting)
	    try {printRes();}
	    catch (const std::invalid_argument& e) {
	        perror(e.what());
	        return -1;
	    }
    }

    if (cmdFlags & _DEBUG_FLAG) printf("Output written successfully to file %s\n", fileOut.c_str());


    //write dataArr to file if requested, used for debugging purposes only
    if(cmdFlags & _PDATA_FLAG){
    	try {printDataArr();}
    	catch (const std::invalid_argument& e) {
	        perror(e.what());
	        return -1;
  		}
    	if (cmdFlags & _DEBUG_FLAG) printf("Data written successfully to file %s\n", fileDataProcessed.c_str());
    }
    return 0;
}

/*
Run apriori algorithm on formatted dataArr using supTable to store freq. patterns, their frequencies
*/
void apriori(){
	//the next size of itemsets to consider
	int setSize = 2;

	//the number of freq. patterns found last iteration
	size_t freqPat = supTable.size();

	//find larger size patterns
	while(freqPat != 0) {
		freqPat = freqCandidates(setSize);
		if(cmdFlags & _DEBUG_FLAG) printf("Number of frequent %d-itemsets found: %u\n", setSize, freqPat);
		setSize++;
	}
}

/*format supTable into desired output format */
void formatOutputNumeric(){
	patOut.reserve(supTable.size());

	//sort each output pattern by numeric value internally
	for(const auto& [itemset, count] : supTable){
		Row fp;
		for(const auto& item : itemset) fp.nums.push_back(stoi(item));
		std::sort(fp.nums.begin(), fp.nums.end());
		fp.count = count;
		patOut.emplace_back(fp);
	}

	//sort frequent patterns overall by numeric value of first element
	std::sort(patOut.begin(), patOut.end(), 
			[&](const auto &first, const auto &second) {
            return first.nums < second.nums ; // sort observation in decreasing order by support
    	});
}

/*
Calculate support for each candidate size-itemset.
Adds frequent patterns to supTable, prunes infrequent items from dataArr.
Returns number of frequent size-itemsets
*/
size_t freqCandidates(size_t size){
	using Itemset = std::vector<std::string>;

	std::unordered_set<Itemset, VectorStringHasher> iItemsets = generateNewItemsets(size);

	std::vector<Itemset> candVector;
	candVector.reserve(iItemsets.size());
	for(const auto& itemset : iItemsets) candVector.emplace_back(itemset);

	std::vector<int> counts(candVector.size(), 0);
	
	size_t numPrevFreqPatterns = supTable.size();

	// prepare cache of all entries that appear in a frequent pattern
	std::unordered_set<std::string> keepEntries;
	keepEntries.reserve(iItemsets.size() * size);

	// bucket candidates by first item
	std::unordered_map<std::string, std::vector<size_t>> bucket; 
	bucket.reserve(candVector.size());
	for(size_t ci = 0; ci < candVector.size(); ci++){
		//add obs index to vector at itemset's key in candVector
		bucket[candVector[ci].front()].push_back(ci);
	}


	//loop through dataArr and populate counts
	int ctr = 0;
	for(const auto& obs : dataArr){
		if(cmdFlags & _DEBUG_FLAG && (ctr%(dataArr.size()/10))==0) printf("Processing observation: %d of %u\n", ctr, dataArr.size());
		ctr++;
		if(obs.size() < size) continue;

		//create set of entries in obs for fast lookup
		std::unordered_set<std::string> obsSet(obs.begin(), obs.end());

		//Loop through entries in obs
		for(const auto& head : obsSet){
			auto it = bucket.find(head);
			//skip entries that are not the head of any candidate set
			if (it == bucket.end()) continue;

			
			//loop through list of candidate indicies that match head string 
			//ci = index of candidate in candVector
			for(size_t ci : it->second){

				bool containsCandidate = true;

	            // check if remaining items in cand also appear in obs
	            for (const auto& cand : candVector[ci]) {
	                //If entry in candidate does not appear in obs, this observation does not contain candidate
	                if (!obsSet.count(cand)) { 
	                	containsCandidate = false; 
	                	break; 
	                }	            
	            }

	            //every entry in candidate is present in obs, iterate count
	            if (containsCandidate) counts[ci]++;
			}
		}
	}

	int ban = 0;
	int i = 0;
	//check count of each candidate itemset, add frequent patterns to supTable and their entries to keepEntries
	for(const auto& itemset : candVector){
		int count = counts[i];
		if (count < minsup) ban++;
		else {
			for(const auto& entry : itemset) keepEntries.insert(entry);
			supTable[itemset] = count;
		}
		i++;
	}
	if(cmdFlags & _DEBUG_FLAG) printf("Number of candidate %u-itemsets pruned before frequency check: %d/%u\n", size, ban, iItemsets.size());

	//loop through dataArr, for each observation:
	//	remove infrequent size-itemsets
	for(auto& obs : dataArr){
		obs.erase(std::remove_if(obs.begin(), obs.end(), 
				  [&](const std::string& s){
                                 return keepEntries.find(s) == keepEntries.end();
                             }), 
				  obs.end());
	}

	dataArr.erase(
	    std::remove_if(dataArr.begin(), dataArr.end(),
	                   [&](const std::vector<std::string>& obs) { return obs.size() <= size; }),
	    dataArr.end()
	);

	return supTable.size() - numPrevFreqPatterns;	
}

/*
Generate candidate newSize-itemsets from all frequent (newSize-1)-itemsets
Only works for newSize >= 2
Returns set containing all lexicographically sorted candidate newSize-itemsets for which all subsets are frequent
*/
std::unordered_set<std::vector<std::string>, VectorStringHasher> 
generateNewItemsets(int newSize){
    using Itemset = std::vector<std::string>;

	std::vector<Itemset> prevItemsets;
	std::unordered_set<Itemset, VectorStringHasher> newIS_Set;
	std::unordered_set<Itemset, VectorStringHasher> newItemsets;

	prevItemsets.reserve(supTable.size());

	int prevSize = newSize-1;

	//populate prevItemsets with all frequent (i-1)-itemsets
	for(const auto& [itemset, count] : supTable){
		if(itemset.size() == prevSize) prevItemsets.emplace_back(itemset);
	}

	newItemsets.reserve(prevItemsets.size());
	newIS_Set.reserve(prevItemsets.size());

	//ensure previous itemsets are lexicographically
	std::sort(prevItemsets.begin(), prevItemsets.end());
	for(const auto& itemset : prevItemsets) newIS_Set.insert(itemset);


	//Generate candidates from valid pairs of prevItemsets and add to newItemsets if all subsets are frequent
	for(size_t i = 0; i < prevItemsets.size(); i++){
		for(size_t j = i+1; j < prevItemsets.size(); j++){
			if(!std::equal(prevItemsets[i].begin(), prevItemsets[i].begin()+prevSize-1, prevItemsets[j].begin())){
				break;
			}

			// ensure candidate string will be in lexicographical order to prevent duplicate patterns
			if(prevItemsets[i][prevSize-1] >= prevItemsets[j][prevSize-1]) continue;

			Itemset candidate;
			candidate.reserve(newSize);
			//add (i-1)-itemset prefix
			candidate.insert(candidate.end(), prevItemsets[i].begin(), prevItemsets[i].end());
			candidate.push_back(prevItemsets[j][prevSize-1]);

			bool allSubsetsFreq = true;
			//loop through all subsets and check if they were counted frequent last iteration
			for(int x = 0; x < newSize; x++){
				Itemset subset;
				subset.resize(prevSize);
				std::copy(candidate.begin(), candidate.begin() + x, subset.begin());
				std::copy(candidate.begin() + x + 1, candidate.end(), subset.begin() + x);
				if(newIS_Set.find(subset) == newIS_Set.end()){
					allSubsetsFreq = false;
					break;
				}
			}
			if(allSubsetsFreq) newItemsets.insert(std::move(candidate));
			
		}
	}
	if(cmdFlags & _DEBUG_FLAG) printf("\nNumber of valid %d-itemsets found: %d\n", newSize, newItemsets.size());
	return newItemsets;
}

/*
Populate cmdFlags, fileIn, and fileOut variables based on given arguments,
Throws invalid_argument exception with descriptive message if it encounters a problem
If debug flag is set, prints subsequent ignored arguments and the files assigned as input and output
*/
void parseInputs(int argc, char* argv[]){
	for(int i = 1; i < argc; i++){

		//parse flags passed anywhere in input
		if(argv[i][0] == '-'){ 
			std::string flag = argv[i];
			//parse debug flag
			if(flag.find("d") != std::string::npos) cmdFlags |= _DEBUG_FLAG;

			//parse print first flag
			else if(flag.find("f") != std::string::npos) cmdFlags |= _PFIRST_FLAG;

			//indicate non-numeric dataset
			else if(flag.find("s") != std::string::npos) cmdFlags |= _NUMERIC_FLAG;

			//parse print data flag
			else if(flag.find("e") != std::string::npos) {
				if(i+1 >= argc) throw std::invalid_argument("Print data flag set but no file given.");
				fileDataProcessed = argv[i+1];
				cmdFlags |= _PDATA_FLAG;
				i++;
			}
		}

		//store first unflagged input as data file, second as minsup, third as output file
		else{ 
			if(fileIn == std::string("")) fileIn = argv[i];
			else if(minsup == -1) minsup = atoi(argv[i]);
			else if(fileOut == std::string("")) fileOut = argv[i];
			else if (cmdFlags & _DEBUG_FLAG) fprintf(stderr, "Arg %d not understood: %s\n", i, argv[i]);		
		}
	}

	cmdFlags ^= _NUMERIC_FLAG;

	if(fileIn == std::string("")) throw std::invalid_argument("No input file given.");

	if(cmdFlags & _DEBUG_FLAG) printf("Input file: %s \nMinsup: %d\nOutput file: %s\n", fileIn.c_str(), minsup, fileOut.c_str());
}

/*
Open data file and parse into dataArr as vector of string vectors
Builds table for frequent 1-itemsets and sorts each observation in dataArr by support count
Throws invalid_argument if there are issues accessing file
If debug flag is set, prints the number of lines read from the file
If print first flag is set, prints the first observation as read from the input file
*/
void readData(){
	std::ifstream dataFile(fileIn);

	if (!dataFile.is_open()) throw std::invalid_argument("Could not open file " + fileIn);
	
	std::string line;
	//loop through dataFile line by line, i tracks line, j tracks place in string
	int i = 0;
	while (std::getline(dataFile, line)){
		dataArr.emplace_back(std::vector<std::string>());

		//loop through observation string, splitting by element, add each to dataArr
		size_t j = 0;
		while(j < line.size()){
			while(j < line.size() && line[j] == ' ') j++;
			if (j >= line.size()) break;

			size_t end = line.find(' ', j);
			if(end == std::string::npos) end = line.size();
			std::vector<std::string> entry = {line.substr(j, end - j)};
			dataArr[i].emplace_back(entry[0]);
			supTable[entry]++;

			j = end + 1;
		}
		i++;
	}

	dataFile.close();
	
	if(cmdFlags & _DEBUG_FLAG){
			printf("Read %d lines from %s\n", i, fileIn.c_str());
	}
	if(cmdFlags & _PFIRST_FLAG){
		printf("First Observation: [");
		std::string obsString;
		for(const auto& entry : dataArr[0]) obsString += (entry + ", ");

		if(!obsString.empty()) obsString.erase(obsString.length()-2);
		printf("%s]\n", obsString.c_str());
	} 

	// build cache of 1-itemset supports (avoids costly redundant recasts to vector)
	cacheSingles.reserve(supTable.size());
	for(const auto& [itemset, count] : supTable) {
		cacheSingles[itemset.front()] = count;
	}

	std::unordered_set<std::string> ifItemsets;

	int ban = 0;

	//identify infrequent itemsets
	if(cmdFlags & _DEBUG_FLAG) printf("List of 1-itemsets being pruned: [");
	for (const auto& [item, count] : cacheSingles) {
	    if (count < minsup) {
	    	ifItemsets.insert(item);
			if(cmdFlags & _DEBUG_FLAG) {
				if(ban != 0) printf(", ");
				printf("%s", item.c_str());
				ban++;
			}
	    }
	}
	if(cmdFlags & _DEBUG_FLAG) printf("]\n");
	

	//loop through dataArr, for each observation:
	//	remove infrequent 1-itemsets, then sort by support count
	for(auto& obs : dataArr){
		for(size_t i = 0; i < obs.size(); i++){
			const auto& entry = obs.at(i);
			//remove infrequent 1-itemsets from dataArr and supTable
			if(ifItemsets.find(entry) != ifItemsets.end()){
				supTable.erase(std::vector<std::string>({entry}));
				obs.erase(obs.begin() + i);
				i--;
			}
		}

		std::sort(obs.begin(), obs.end(), 
			[&](const std::string &first, const std::string &second) {
            return cacheSingles[first] > cacheSingles[second]; // sort observation in decreasing order by support
        });
	}

	dataArr.erase(
	    std::remove_if(dataArr.begin(), dataArr.end(),
	                   [](const std::vector<std::string>& obs) { return obs.empty(); }),
	    dataArr.end()
	);

}


/*
Prints results to file in lexicographical order
Throws invalid_argument exception if there are issues writing to file
*/
void printRes(){
	std::ofstream outputFile(fileOut);

	if(!outputFile.is_open()) throw std::invalid_argument("Could not write to file " + fileOut);
	else{
		for(const auto& [itemset, count] : supTable){
			for(auto& item : itemset) outputFile << item << " ";
			outputFile << "(" << std::to_string(count) << ")" << std::endl;
		}
		outputFile.close();
	}
}

/*
Prints results to file in numeric order
Throws invalid_argument exception if there are issues writing to file
*/
void printResNumeric(){
	formatOutputNumeric();
	std::ofstream outputFile(fileOut);

	if(!outputFile.is_open()) throw std::invalid_argument("Could not write to file " + fileOut);
	else{
		for(const auto& row : patOut){
			for(auto& num : row.nums) outputFile << std::to_string(num) << " ";
			outputFile << "(" << std::to_string(row.count) << ")" << std::endl;
		}
		outputFile.close();
	}
}

/*
print dataArr to output file for debugging
*/
void printDataArr(){
	std::ofstream pDataFile(fileDataProcessed);

	if(!pDataFile.is_open()) throw std::invalid_argument("Could not write to file " + fileOut);
	else{
		for(const auto& obs : dataArr){
			for(std::string entry : obs) pDataFile << entry << " ";
			pDataFile << std::endl;
		}
		pDataFile.close();
	}
}
