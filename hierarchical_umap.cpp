#include "hierarchical_umap.h"

namespace py = pybind11;
using namespace std;


vector<utils::SparseData> humap::create_sparse(int n, const vector<int>& rows, const vector<int>& cols, const vector<float>& vals)
{
	vector<utils::SparseData> sparse(n, utils::SparseData());

	for( int i = 0; i < rows.size(); ++i )
		sparse[rows[i]].push(cols[i], vals[i]);


	return sparse;
}

vector<vector<float>> humap::convert_to_vector(const py::array_t<float>& v)
{
	py::buffer_info bf = v.request();
	float* ptr = (float*) bf.ptr;

	vector<vector<float>> vec(bf.shape[0], vector<float>(bf.shape[1], 0.0));
	for (int i = 0; i < vec.size(); ++i)
	{
		for (int j = 0; j < vec[0].size(); ++j)
		{
			vec[i][j] = ptr[i*vec[0].size() + j];
		}
	}

	return vec;
}


std::map<std::string, float> humap::convert_dict_to_map(py::dict dictionary) 
{
	std::map<std::string, float> result;

	for( std::pair<py::handle, py::handle> item: dictionary ) {

		std::string key = item.first.cast<std::string>();
		float value = item.second.cast<float>();

		result[key] = value; 

	}
	return result;
}

void humap::split_string( string const& str, vector<humap::StringRef> &result, char delimiter)
{
    enum State { inSpace, inToken };

    State state = inSpace;
    char const*     pTokenBegin = 0;    // Init to satisfy compiler.
    for( string::const_iterator it = str.begin(); it != str.end(); ++it )
    {
        State const newState = (*it == delimiter? inSpace : inToken);
        if( newState != state )
        {
            switch( newState )
            {
            case inSpace:
                result.push_back( StringRef( pTokenBegin, &*it - pTokenBegin ) );
                break;
            case inToken:
                pTokenBegin = &*it;
            }
        }
        state = newState;
    }
    if( state == inToken )
    {
        result.push_back( StringRef( pTokenBegin, &*str.end() - pTokenBegin ) );
    }
}


int humap::dfs(int u, int n_neighbors, bool* visited, vector<int>& cols, vector<float>& sigmas,
			   float* strength, int* owners, int last_landmark)
{
	visited[u] = true;

	if( sigmas[u] >= sigmas[last_landmark] )
		return u;
	else if( *(strength + u) != -1 )
		return *(owners + u);

	int* neighbors = new int[n_neighbors*sizeof(int)];
	for( int j = 0; j < n_neighbors; ++j ) {
		neighbors[j] = cols[u*n_neighbors + j];
	}	

	for( int i = 1; i < n_neighbors; ++i ) {
		int neighbor = neighbors[i];

		if( *(visited + neighbor) ) {

			int landmark = humap::dfs(neighbor, n_neighbors, visited, cols, sigmas, 
									  strength, owners, last_landmark);
			if( landmark != -1 ) {
				free(neighbors);
				neighbors = nullptr;
				return landmark;
			}
		}
	}


	if( neighbors )
		free(neighbors);
	return -1;


}

int humap::depth_first_search(int n_neighbors, int* neighbors, vector<int>& cols, vector<float>& sigmas,
							  float* strength, int* owners, int last_landmark)
{
	bool* visited = new bool[sigmas.size()*sizeof(bool)];
	memset(visited, false, sigmas.size()*sizeof(bool));

	for( int i = 1; i < n_neighbors; ++i ) {

		int neighbor = *(neighbors + i);

		if( !*(visited + neighbor) ) {

			int landmark = humap::dfs(neighbor, n_neighbors, visited, cols, sigmas, strength, owners, last_landmark);
			if( landmark != -1 )  
				return landmark;

		}
	}

	return -1;
}


void humap::associate_to_landmarks(int n, int n_neighbors, int* indices, vector<int>& cols, vector<float>& sigmas,
								   float* strength, int* owners, int last_landmark, 
								   Eigen::SparseMatrix<float, Eigen::RowMajor>& graph)
{
	int* neighbors = new int[n_neighbors*sizeof(int)];

	for( int i = 0; i < n; ++i ) {
		int index = indices[i];

		for( int j = 0; j < n_neighbors; ++j ) {
			neighbors[j] = cols[index*n_neighbors + j];
		}

		int nn = neighbors[1];

		if( sigmas[nn] >= sigmas[last_landmark] ) {
			*(strength + index) = graph.coeffRef(nn, index);
			*(owners + index) = nn;
		} else {
			if( *(strength + nn) != -1 ) {
				*(strength + index) = graph.coeffRef(*(owners + nn), index);
				*(owners + index) = *(owners + nn);
			} else {

				int landmark = humap::depth_first_search(n_neighbors, neighbors, cols, sigmas, 
					                                     strength, owners, last_landmark);

				if( landmark != -1 ) {
					cout << "Found a landmark :)" << endl;
					*(strength + index) = graph.coeffRef(landmark, index);
					*(owners + index) = landmark;
				} else 
					throw runtime_error("Did not find a landmark");
			}
		}
	}

	free(neighbors);
}

void humap::HierarchicalUMAP::add_similarity(int index, int i, int n_neighbors, vector<int>& cols,  
					Eigen::SparseMatrix<float, Eigen::RowMajor>& graph, vector<vector<float>>& knn_dists,
					std::vector<std::vector<float> >& membership_strength, std::vector<std::vector<int> >& indices,
					std::vector<std::vector<float> >& distance, int* mapper, 
					float* elements, vector<vector<int>>& indices_nzeros, int n)
{
	using clock = chrono::system_clock;
	using sec = chrono::duration<double>;
	// cout << "falha 3" << endl;
	std::vector<int> neighbors;

	for( int j = 1; j < n_neighbors; ++j ) {
		neighbors.push_back(cols[i*n_neighbors + j]);
	}
	// cout << "falha 4" << endl;
	//#pragma omp parallel for default(shared) schedule(dynamic, 50)
	// #pragma omp parallel for default(shared) schedule(dynamic, 100)
	for( int j = 0; j < neighbors.size(); ++j ) {
		// cout << "falha 6" << endl;
		int neighbor = neighbors[j];

		if( membership_strength[neighbor].size() == 0 ) {
			// cout << "falha 5" << endl;

			// #pragma omp critical(initing_neighbors)
			// {
				membership_strength[neighbor].push_back(graph.coeffRef(i, neighbor));
				// cout << "falha 7" << endl;	
				distance[neighbor].push_back(knn_dists[i][j+1]);//(knn_dists[i*n_neighbors + (j+1)]);
				// cout << "falha 8" << endl;
				indices[neighbor].push_back(i);
			// }
		} else {
			float ms2 = graph.coeffRef(i, neighbor);
			float d2 = knn_dists[i][j+1];//[i*n_neighbors + (j+1)];
			int ind2 = i;
				
			for( int count = 0; count < membership_strength[neighbor].size(); ++count ) {

				float ms1 = membership_strength[neighbor][count];
				float d1 = distance[neighbor][count];
				int ind1 = indices[neighbor][count];
	

				float s = (std::min(ms1*d1, ms2*d2)/std::max(ms1*d1, ms2*d2))/(n_neighbors-1);

				// test as map
				if( *(mapper + ind1) != -1 ) {

					int u = *(mapper + ind1);
					int v = *(mapper + ind2);

					*(elements + u*n + v) += s;
					*(elements + v*n + u) += s;

					indices_nzeros[u].push_back(v);
					indices_nzeros[v].push_back(u);

				}
			}
			membership_strength[neighbor].push_back(ms2);
			distance[neighbor].push_back(d2);
			indices[neighbor].push_back(ind2);
		}
	}
}

void humap::tokenize(std::string &str, char delim, std::vector<std::string> &out)
{
	size_t start;
	size_t end = 0;

	while ((start = str.find_first_not_of(delim, end)) != std::string::npos)
	{
		end = str.find(delim, start);
		out.push_back(str.substr(start, end - start));
	}
}

humap::SparseComponents humap::HierarchicalUMAP::create_sparse(int n, int n_neighbors, float* elements, vector<vector<int>>& indices_nzeros)
{

	vector<int> cols;
	vector<int> rows;
	vector<float> vals;

	int* current = new int[n*sizeof(int)];
	memset(current, 0, n * sizeof(int));

	for( int i = 0; i < n; ++i ) {
		bool flag = true;
		for( int j = 0; j < indices_nzeros[i].size(); ++j ) {
			int index = indices_nzeros[i][j];

			if( *(current + index) )
				continue;

			*(current + index) = 1;
			if( *(elements + i*n + index) != 0.0 ) {				
				rows.push_back(i);
				cols.push_back(index);
				if( i == index )
					flag =false;
				vals.push_back(1 - *(elements + i*n + index));			
			}
		}

 		for( int j = 0; j < n_neighbors+5; ++j ) {
			if( *(elements + i*n + j) == 0.0 && i != j) {				
				rows.push_back(i);
				cols.push_back(j);
				vals.push_back(1);
			} 
		}

		for( int j = 0; j < indices_nzeros[i].size(); ++j ){
			*(current + indices_nzeros[i][j]) = 0;
		}
		 if(  flag ) {
		 	rows.push_back(i);
				cols.push_back(i);
				vals.push_back(0);
		 }
	}

	return humap::SparseComponents(rows, cols, vals);
}



humap::SparseComponents humap::HierarchicalUMAP::sparse_similarity(int n, int n_neighbors, vector<int>& greatest, vector<int> &cols, 
																   Eigen::SparseMatrix<float, Eigen::RowMajor>& graph, vector<vector<float>>& knn_dists) 
{

	using clock = chrono::system_clock;
	using sec = chrono::duration<double>;

	std::vector<std::vector<int> > indices_sim;
	std::vector<std::vector<float> > membership_strength;
	std::vector<std::vector<float> > distance_sim;

	int* mapper = new int[n * sizeof(int)];
	memset(mapper, -1, n * sizeof(int));

	for( int i = 0; i < greatest.size(); ++i )
		mapper[greatest[i]] = i;


	for( int i = 0; i < n; ++i ) {
		indices_sim.push_back(std::vector<int>());
		membership_strength.push_back(std::vector<float>());
		distance_sim.push_back(std::vector<float>());
	}

	float* elements = new float[greatest.size()*greatest.size()*sizeof(float)];
	memset(elements, 0.0, greatest.size()*greatest.size()*sizeof(float));


	int* non_zeros = new int[greatest.size() * sizeof(int)];
	memset(non_zeros, 0, greatest.size() * sizeof(int));

	vector<vector<int>> indices_nzeros(greatest.size(), vector<int>());

	

	for( int i = 0; i < greatest.size(); ++i ) {


		add_similarity(i, greatest[i], n_neighbors, cols, graph, knn_dists, membership_strength, 
			indices_sim, distance_sim, mapper, elements, indices_nzeros, greatest.size());

		// data_dict[std::to_string(i)+'_'+std::to_string(i)] = 1.0;
	}
	auto begin = clock::now();
	humap::SparseComponents sc = this->create_sparse(greatest.size(), n_neighbors, elements, indices_nzeros);
	sec duration = clock::now() - begin;
	cout << "Time for sparse components: " << duration.count() << endl;


	free(elements);
	free(non_zeros);
	free(mapper);

	return sc;
}


vector<float> humap::HierarchicalUMAP::update_position(int i, int n_neighbors, vector<int>& cols, vector<float>& vals,
													   umap::Matrix& X, Eigen::SparseMatrix<float, Eigen::RowMajor>& graph)
{
	std::vector<int> neighbors;

	for( int j = 0; j < n_neighbors; ++j ) {
		neighbors.push_back(cols[i*n_neighbors + j]);
	}


	vector<float> u = X.dense_matrix[i];

	vector<float> mean_change(X.shape(1), 0);
	for( int j = 0; j < neighbors.size(); ++j ) {
		int neighbor = neighbors[j];

		vector<float> v = X.dense_matrix[neighbor];

		vector<float> temp(v.size(), 0.0);
		for( int k = 0; k < temp.size(); ++k )
			temp[k] = graph.coeffRef(i, neighbor)*(v[k]-u[k]);

		transform(mean_change.begin(), mean_change.end(), 
				  temp.begin(), mean_change.begin(), plus<float>());
	}

	transform(mean_change.begin(), mean_change.end(), mean_change.begin(), [n_neighbors](float& c){
		return c/(n_neighbors);
	});

	transform(u.begin(), u.end(), mean_change.begin(), u.begin(), plus<float>());

	return u;
}

double humap::sigmoid(double input) 
{

	return 1.0/(1.0 + exp(-input));
}

void humap::softmax(vector<double>& input, size_t size) {

	vector<double> exp_values(input.size(), 0.0);
	for( int i = 0; i < exp_values.size(); ++i ) {
		exp_values[i] = exp(input[i]);
	}
	double sum_exp = accumulate(exp_values.begin(), exp_values.end(), 0.0);
	for( int i = 0; i < exp_values.size(); ++i ) {
		input[i] = exp(input[i])/sum_exp;
	}	
}


void humap::HierarchicalUMAP::fit(py::array_t<float> X, py::array_t<int> y)
{
	using clock = chrono::system_clock;
	using sec = chrono::duration<double>;


	// const auto before = clock::now();

	// cout << "Size: " << X.request().shape[0] << ", " << X.request().shape[1] << endl;
	// cout << "Similarity Method: " << this->similarity_method << endl;
	// cout << "Number of levels: " << this->percents.size() << endl;
	// for( int level = 0; level < this->percents.size(); ++level )
	// 	printf("Level %d: %.2f of the instances from level %d.\n", (level+1), this->percents[level], level);
	// cout << "KNN algorithm: " << this->knn_algorithm << endl;


	// const sec duration = clock::now() - before;
	// cout << "it took "  << duration.count() << "s" << endl;


	// return;
	auto hierarchy_before = clock::now();


	auto before = clock::now();
	umap::Matrix first_level(humap::convert_to_vector(X));
	sec duration = clock::now() - before;

	this->hierarchy_X.push_back(first_level);
	this->hierarchy_y.push_back(vector<int>((int*)y.request().ptr, (int*)y.request().ptr + y.request().shape[0]));

	// if( this->knn_algorithm == "FAISS_IVFFlat" && first_level.size() < 10000 ) {
	// 	this->knn_algorithm = "FAISS_Flat";
	// }

	umap::UMAP reducer = umap::UMAP("euclidean", this->knn_algorithm);

	if( this->verbose ) {
		cout << "\n\n*************************************************************************" << endl;
		cout << "*********************************LEVEL 0*********************************" << endl;
		cout << "*************************************************************************" << endl;
	}

	before = clock::now();
	reducer.fit_hierarchy(this->hierarchy_X[0]);
	duration = clock::now() - before;
	cout << "Fitting for first level: " << duration.count() << endl;
	cout << endl;

	this->reducers.push_back(reducer);

	int* indices = new int[sizeof(int)*X.size()];
	iota(indices, indices+X.size(), 0);

	int* owners = new int[sizeof(int)*X.size()];
	memset(owners, -1, sizeof(int)*X.size());

	float* strength = new float[sizeof(float)*X.size()];
	memset(strength, -1.0, sizeof(float)*X.size());




	this->metadata.push_back(humap::Metadata(indices, owners, strength, X.size()));


	Eigen::SparseMatrix<float, Eigen::RowMajor> graph = this->reducers[0].get_graph();

	vector<vector<float>> knn_dists = this->reducers[0].knn_dists();

	for( int level = 0; level < this->percents.size(); ++level ) {

		auto level_before = clock::now();

		int n_elements = (int) (this->percents[level] * this->hierarchy_X[level].size());
		



		auto begin_sampling = clock::now();
		vector<float> values(this->reducers[level].sigmas().size(), 0.0);
		float mean_sigma = 0.0;
		float sum_sigma = 0.0;
		float max_sigma = -1;

		for( int i = 0; i < values.size(); ++i ) {
			max_sigma = max(max_sigma, this->reducers[level].sigmas()[i]);
			sum_sigma += fabs(this->reducers[level].sigmas()[i]);
 		}

		vector<double> probs(this->reducers[level].sigmas().size(), 0.0);
		for( int i = 0; i < values.size(); ++i ) {
			// probs[i] = this->reducers[level].sigmas()[i];
			// probs[i] = this->reducers[level].sigmas()[i]/max_sigma;
			probs[i] = this->reducers[level].sigmas()[i]/sum_sigma;
			
			// probs[i] = humap::sigmoid(this->reducers[level].sigmas()[i]);
 		}
		
   		humap::softmax(probs, this->reducers[level].sigmas().size());

		py::module np = py::module::import("numpy");
		py::object choice = np.attr("random");
		py::object indices_candidate = choice.attr("choice")(py::cast(probs.size()), py::cast(n_elements), py::cast(false),	py::cast(probs));
		vector<int> possible_indices = indices_candidate.cast<vector<int>>();

   		vector<int> s_indices = utils::argsort(possible_indices);
   		for( int i = 1; i < s_indices.size(); ++i ) {
   			if( possible_indices[s_indices[i-1]] == possible_indices[s_indices[i]] ) {
   				cout << "OLHA ACHEI UM REPETIDO :)" << endl;
   			}
   		}

		vector<int> greatest = possible_indices;

		this->_sigmas.push_back(this->reducers[level].sigmas());

		this->_indices.push_back(greatest);

		umap::Matrix data;	

		if( this->verbose ) {
			cout << "\n\n*************************************************************************" << endl;
			cout << "*********************************LEVEL "<< (level+1) << "*********************************" << endl;
			cout << "*************************************************************************" << endl;
		}
		
		if( this->verbose )
			cout << "Level " << (level+1) << ": " << n_elements << " data samples." << endl;


		if( this->verbose )
			cout << "Computing level" << endl;


		if( this->similarity_method == "similarity" ) {

			vector<vector<float>> dense;


			for( int i = 0; i < greatest.size(); ++i ) {
					
				vector<float> row = update_position(greatest[i], this->n_neighbors, this->reducers[level].cols, 
					this->reducers[level].vals, this->hierarchy_X[level], graph);
				dense.push_back(row);
			}


			data = umap::Matrix(dense);
			reducer = umap::UMAP("euclidean", this->knn_algorithm);



		} else if( this->similarity_method == "precomputed" ) {

			auto sparse_before = clock::now();
			SparseComponents triplets = this->sparse_similarity(this->hierarchy_X[level].size(), this->n_neighbors,
																 greatest, this->reducers[level].cols,
																 graph, knn_dists);
			sec sparse_duration = clock::now() - sparse_before;

			cout << "Sparse Matrix: " << sparse_duration.count() << endl;

			auto eigen_before = clock::now();
			// Eigen::SparseMatrix<float, Eigen::RowMajor> sparse = utils::create_sparse(triplets.rows, triplets.cols, triplets.vals,
			// 																		  n_elements, n_neighbors*4);


			vector<utils::SparseData> sparse = humap::create_sparse(n_elements, triplets.rows, triplets.cols, triplets.vals);
			sec eigen_duration = clock::now() - eigen_before;
			cout << "Constructing eigen matrix: " << eigen_duration.count() << endl;
			cout << endl;

			data = umap::Matrix(sparse);
			reducer = umap::UMAP("precomputed", this->knn_algorithm);
		} else {


			reducer = umap::UMAP("euclidean", this->knn_algorithm);


		}


		if( this->verbose )
			cout << "Fitting hierarchy level" << endl; 

		auto fit_before = clock::now();
		reducer.fit_hierarchy(data);
		sec fit_duration = clock::now() - fit_before;
		cout << "Fitting level " << (level+1) << ": " << fit_duration.count() << endl;
		cout << endl;

		// int n = 0;
		// for( size_t i = 0; i < this->metadata[level].size; ++i )
		// 	if( this->metadata[level].strength[i] == -1.0 )
		// 		n++;

		// int* indices_not_associated = new int[sizeof(int)*n];
		// for( size_t i = 0, j = 0; i < this->metadata[level].size; ++i )
		// 	if( this->metadata[level].strength[i] == -1.0 )
		// 		*(not_associated + j++) = i;

		// int last_landmark = greatest[greatest.size()-1];

		// humap::associate_to_landmarks(n, this->n_neighbors, indices_not_associated, this->reducer[level].cols,
		// 							  this->reducer[level].sigmas, this->metadata[level].strength,
		// 							  this->metadata[level].owners, last_landmark, graph);

		if( this->verbose )
			cout << "Appending information for the next hierarchy level" << endl;

		this->metadata.push_back(Metadata(greatest.data(), nullptr, nullptr, greatest.size()));//vector<int>(greatest.size(), -1), vector<float>(greatest.size(), -1.0)));
		this->reducers.push_back(reducer);
		this->hierarchy_X.push_back(data);
		this->hierarchy_y.push_back(utils::arrange_by_indices(this->hierarchy_y[level], greatest));


		knn_dists = reducer.knn_dists();
		graph = reducer.get_graph();

		Eigen::VectorXf result = graph * Eigen::VectorXf::Ones(graph.cols());
		vector<float> my_vec(&result[0], result.data() + result.size());


		sec level_duration = clock::now() - level_before;
		cout << "Level construction: " << level_duration.count() << endl;
		cout << endl;
	}




	sec hierarchy_duration = clock::now() - hierarchy_before;
	cout << endl;
	cout << "Hierarchy construction: " << hierarchy_duration.count() << endl;
	cout << endl;



	for( int i = 0; i < this->hierarchy_X.size(); ++i ) {
		cout << "chguei aqui 1 " << endl;
		Eigen::SparseMatrix<float, Eigen::RowMajor> graph = this->reducers[i].get_graph();		
		int n_vertices = graph.cols();
		cout << "chguei aqui 2 " << endl;

		if( this->n_epochs == -1 ) {
			if( graph.rows() <= 10000 )
				n_epochs = 500;
			else 
				n_epochs = 200;

		}
		cout << "chguei aqui 3 " << endl;
		if( !graph.isCompressed() )
			graph.makeCompressed();
		// graph = graph.pruned();
		cout << "chguei aqui 4 " << endl;
		float max_value = graph.coeffs().maxCoeff();
		graph = graph.pruned(max_value/(float)n_epochs, 1.0);


		cout << "chguei aqui 5 " << endl;


		vector<vector<float>> embedding = this->reducers[i].spectral_layout(this->hierarchy_X[i], graph, this->n_components);


		cout << "chguei aqui 6 " << endl;
		vector<int> rows, cols;
		vector<float> data;

		
		cout << "chguei aqui 7 " << endl;
		tie(rows, cols, data) = utils::to_row_format(graph);

		cout << "chguei aqui 8 " << endl;
		vector<float> epochs_per_sample = this->reducers[i].make_epochs_per_sample(data, this->n_epochs);
		// cout << "\n\nepochs_per_sample: " << epochs_per_sample.size() << endl;

		cout << "chguei aqui 9 " << endl;
		// for( int j = 0; j < 20; ++j )
		// 	printf("%.4f ", epochs_per_sample[j]);

		// cout << endl << endl;



		cout << "chguei aqui 10 " << endl;
		vector<float> min_vec, max_vec;
		// printf("min and max values:\n");
		for( int j = 0; j < this->n_components; ++j ) {




			min_vec.push_back((*min_element(embedding.begin(), embedding.end(), 
				[j](vector<float> a, vector<float> b) {							
					return a[j] < b[j];
				}))[j]);

			max_vec.push_back((*max_element(embedding.begin(), embedding.end(), 
				[j](vector<float> a, vector<float> b) {
					return a[j] < b[j];
				}))[j]);

		}
		cout << "chguei aqui 11 " << endl;
		vector<float> max_minus_min(this->n_components, 0.0);
		transform(max_vec.begin(), max_vec.end(), min_vec.begin(), max_minus_min.begin(), [](float a, float b){ return a-b; });
		cout << "chguei aqui 12 " << endl;

		for( int j = 0; j < embedding.size(); ++j ) {

			transform(embedding[j].begin(), embedding[j].end(), min_vec.begin(), embedding[j].begin(), 
				[](float a, float b) {
					return 10*(a-b);
				});

			transform(embedding[j].begin(), embedding[j].end(), max_minus_min.begin(), embedding[j].begin(),
				[](float a, float b) {
					return a/b;
				});
		}
		cout << "chguei aqui 13 " << endl;

		// printf("\n\nNOISED EMBEDDING:\n");

		// for( int j = 0; j < 10; ++j ) {
		// 	if(  j < 10 )
		// 		printf("%.4f %.4f\n", embedding[j][0], embedding[j][1]);
		// }
		// printf("------------------------------------------------------------------\n");

		py::module scipy_random = py::module::import("numpy.random");
		py::object randomState = scipy_random.attr("RandomState")(this->random_state);

		py::object rngStateObj = randomState.attr("randint")(numeric_limits<int>::min(), numeric_limits<int>::max(), 3);
		vector<long> rng_state = rngStateObj.cast<vector<long>>();

		printf("Embedding a level with %d data samples\n", embedding.size());
		before = clock::now(); 
		vector<vector<float>> result = this->reducers[i].optimize_layout_euclidean(
			embedding,
			embedding,
			rows,
			cols,
			this->n_epochs,
			n_vertices,
			epochs_per_sample,
			rng_state);
		duration = clock::now() - before;
		cout << endl << "It took " << duration.count() << " to embed." << endl;

		this->embeddings.push_back(result);

	}

}

py::array_t<float> humap::HierarchicalUMAP::get_sigmas(int level)
{
	if( level >= this->hierarchy_X.size()-1 || level < 0 )
		throw new runtime_error("Level out of bounds.");

	return py::cast(this->_sigmas[level]);
}

py::array_t<int> humap::HierarchicalUMAP::get_indices(int level)
{
	if( level >= this->hierarchy_X.size()-1 || level < 0 )
		throw new runtime_error("Level out of bounds.");

	return py::cast(this->_indices[level]);
}


py::array_t<int> humap::HierarchicalUMAP::get_labels(int level)
{
	if( level == 0 )  
		throw new runtime_error("Sorry, we won't me able to return all the labels!");

	if( level >= this->hierarchy_X.size() || level < 0 )
		throw new runtime_error("Level out of bounds.");


	return py::cast(this->hierarchy_y[level]);
}

py::array_t<float> humap::HierarchicalUMAP::get_embedding(int level)
{
	if( level >= this->hierarchy_X.size() || level < 0 )
		throw new runtime_error("Level out of bounds.");

	return py::cast(this->embeddings[level]);
}	


Eigen::SparseMatrix<float, Eigen::RowMajor> humap::HierarchicalUMAP::get_data(int level)
{
	if( level == 0 )  
		throw new runtime_error("Sorry, we won't me able to return all dataset! Please, project using UMAP.");

	if( level >= this->hierarchy_X.size() || level < 0 )
		throw new runtime_error("Level out of bounds.");

	return utils::create_sparse(this->hierarchy_X[level].sparse_matrix, this->hierarchy_X[level].size(), (int) this->n_neighbors*2.5);
}