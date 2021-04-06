//includes
#include "ssa.hpp"
#include <math.h>
SSA::SSA(Benchmark *benchmark, unsigned int searchAgentsCount, unsigned int maximumIterations, std::vector<int>ind_val,  double **positions_inicial)
{
	benchmark_m = benchmark;
	searchAgentsCount_m = searchAgentsCount;
	maximumIterations_m = maximumIterations;

	ind_val_m = ind_val;

	boundaries_m = benchmark_m->getBoundaries();
	dimension_m = benchmark_m->GetDimension();
	convergenceCurve_m = Utils::Create1DZeroArray(maximumIterations_m);



	//Initialize the positions of search agents
	positions_m = positions_inicial;

	x_score = Utils::Create1DArray(searchAgentsCount);
	
	best_score = std::numeric_limits<double >::infinity();

}

SSA::~SSA() {
	for (register unsigned int agentId = 0; agentId < searchAgentsCount_m; agentId++) {
		delete positions_m[agentId];
	}

	delete convergenceCurve_m;
}
void SSA::fitness_inicial(std::vector<std::vector<std::vector<cv::KeyPoint>>> bestKey, std::vector<std::string> imagens_src, cv::Mat im360, int rows, int cols, std::vector<std::vector<int>> indices) {
	double fitness;
	int cont = 0;
//#pragma omp parallel for
	for (register int agentIndex = 0; agentIndex < searchAgentsCount_m; agentIndex++) {
		Utils::Clip1DArray(positions_m[agentIndex], dimension_m, boundaries_m);

		//Calculate objective function for each search agent

		fitness = benchmark_m->fitness(positions_m[agentIndex], bestKey, imagens_src, im360, rows, cols, indices);

		//Update Alpha, Beta, and Delta
		if (fitness < x_score[agentIndex]) {
			x_score[agentIndex] = fitness; // Update alpha
			
			//std::copy(&positions_m[agentIndex][0], &positions_m[agentIndex][dimension_m], &alphaPosition_m[0]);
		}
		

	}

	
}
void SSA::calculateFitness(std::vector<std::vector<std::vector<cv::KeyPoint>>> bestKey, std::vector<std::string> imagens_src, cv::Mat im360, int rows, int cols, std::vector<std::vector<int>> indices, double *best_pos, double best_score, int it, double c1)
{
	double fitness;
	double W = 1;
#pragma omp parallel for //morceguinhos
	for (register int agentIndex = 0; agentIndex < searchAgentsCount_m; agentIndex++)
	{

		if (agentIndex <= searchAgentsCount_m / 2)
		{
			for (register unsigned int variable = 0; variable < dimension_m; variable++)
			{
				double c2 = Utils::GenerateRandomNumber();
				double c3 = Utils::GenerateRandomNumber();

				if (c3 < 0.5)
				{
					positions_m[agentIndex][variable] = W * best_pos[variable] + c1 * ((boundaries_m[variable].upperBound - boundaries_m[variable].lowerBound)*c2 + boundaries_m[variable].lowerBound);
				}
				else {
					positions_m[agentIndex][variable] = W * best_pos[variable] - c1 * ((boundaries_m[variable].upperBound - boundaries_m[variable].lowerBound)*c2 + boundaries_m[variable].lowerBound);
				}
			}
		}
		else if (agentIndex > searchAgentsCount_m / 2 && agentIndex < searchAgentsCount_m + 1)
		{
			double *X_0 = positions_m[agentIndex - 1];
			double *X_1 = positions_m[agentIndex];
			for (register unsigned int variable = 0; variable < dimension_m; variable++)
			{
				positions_m[agentIndex][variable] = (W * X_0[variable] + W * X_1[variable]) / 2;
			}

		}


	}
#pragma omp parallel for //morceguinhos
	for (register int agentIndex = 0; agentIndex < searchAgentsCount_m; agentIndex++)
	{
		int *IndUB = new int[dimension_m];
		int *IndLB = new int[dimension_m];

		for (register unsigned int variable = 0; variable < dimension_m; variable++)
		{
			if (positions_m[agentIndex][variable] > boundaries_m[variable].upperBound)
			{

				IndUB[variable] = 1;
			}
			else {

				IndUB[variable] = 0;
			}
			if (positions_m[agentIndex][variable] < boundaries_m[variable].lowerBound)
			{

				IndLB[variable] = 1;
			}
			else {

				IndLB[variable] = 0;
			}
			int non_sum = 0;
			int sum = 1;
			if (IndLB[variable] + IndUB[variable] > 0) {
				non_sum = 0;
				sum = 1;

			}
			else {
				non_sum = 1;
				sum = 0;

			}
			positions_m[agentIndex][variable] = positions_m[agentIndex][variable] * (non_sum)+boundaries_m[variable].upperBound*(IndUB[variable]) + boundaries_m[variable].lowerBound*IndLB[variable];
		}
		fitness = benchmark_m->fitness(positions_m[agentIndex], bestKey, imagens_src, im360, rows, cols, indices);
		if (fitness <= best_score)
		{
			best_score = fitness;
			best_pos = positions_m[agentIndex];
		}

	}



	
}
double* SSA::Evaluate(bool debug, std::vector<std::vector<std::vector<cv::KeyPoint>>> bestKey, std::vector<std::string> imagens_src, cv::Mat im360, std::vector<std::vector<int>> indices) {
	cv::Mat image1 = cv::imread(imagens_src[0]);
	// A avalia a popula��o inicial
	fitness_inicial(bestKey, imagens_src, im360, image1.rows, image1.cols, indices);
	for (register int agentIndex = 0; agentIndex < searchAgentsCount_m; agentIndex++)
	{
		std::cout << x_score[agentIndex] << std::endl;
	}
	
	double *best_pos;
	double *x_score_ordenado;

	std::vector<int>Ind_Salp_Ordenado;
	Ind_Salp_Ordenado.resize(searchAgentsCount_m);
	x_score_ordenado = x_score;
	Utils::sortArr(x_score_ordenado, searchAgentsCount_m, Ind_Salp_Ordenado);// ordenar em ordem crescente

	double **positions_Salp_Ordenado = new double*[searchAgentsCount_m];
	for (int y = 0; y < searchAgentsCount_m; y++)
	{
		positions_Salp_Ordenado[y] = new double[dimension_m];
		positions_Salp_Ordenado[y] = positions_m[Ind_Salp_Ordenado[y]];

	}
	best_score = x_score_ordenado[0];
	best_pos = positions_Salp_Ordenado[0];


	auto start_time = std::chrono::high_resolution_clock::now();
#pragma omp parallel for
	for (register int iteration = 0; iteration < maximumIterations_m; iteration++) {

		double c1 = 2 * exp(-pow(4 * (iteration / maximumIterations_m), 2));

		calculateFitness(bestKey, imagens_src, im360, image1.rows, image1.cols, indices, best_pos, best_score, iteration, c1);

		convergenceCurve_m[iteration] = best_score;
		best_positions_m = best_pos;

		if (debug && (iteration % 1 == 0)) {
			std::cout << "At iteration " << iteration << " the best fitness is "
				<< std::setprecision(3)
				<< best_score << std::endl;
		}

	}

	auto finish_time = std::chrono::high_resolution_clock::now();
	executionTime_m = std::chrono::duration_cast<std::chrono::nanoseconds>(finish_time - start_time).count() * 1e-9;

	return convergenceCurve_m;
}
std::ostream& operator << (std::ostream& os, const SSA *ssa) {
	os << std::scientific
		<< std::setprecision(9)
		<< "Benchmark: " << ssa->benchmark_m->GetName() << std::endl
		<< "SSA position = ";

	for (register unsigned int variable = 0; variable < ssa->dimension_m; variable++) {
		os << ssa->best_positions_m[variable] << " ";
	}

	os << std::endl
		<< "Alpha score (Fitness) = " << ssa->best_score << std::endl
		<< "Time = " << ssa->executionTime_m << " seconds";



	return os;
}
