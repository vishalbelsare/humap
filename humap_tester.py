import numpy as np 
import pandas as pd
import seaborn as sns
import hierarchical_umap as h_umap 
from scipy.sparse import load_npz, csr_matrix
import time
import umap
import matplotlib.pyplot as plt
import demap
import math

from tqdm import tqdm
from sklearn.preprocessing import normalize
from sklearn.neighbors import NearestNeighbors
from sklearn.datasets import fetch_openml
from sklearn.manifold import trustworthiness
from sklearn.utils import check_random_state, check_array



def scale(value, leftMin, leftMax, rightMin, rightMax):

	leftSpan = leftMax - leftMin 
	rightSpan = rightMax - rightMin

	valueScaled = float(value - leftMin) / float(leftSpan)

	return rightMin + (valueScaled * rightSpan)


def transform_sizes(values, minValue, maxValue, rightMin=3, rightMax=50):


	areas = []
	for value in values:
		areas.append(scale(value, minValue, maxValue, rightMin, rightMax))

	areas = np.array(areas)

	return (4.*areas)/math.pi 






def compute_trustworthiness(X, X_emb, Khigh=30):

	ks = np.zeros(Khigh)
	trust = np.zeros(Khigh)
	for i in tqdm(range(1, Khigh+1)):
		ks[i-1] = i
		trust[i-1] = trustworthiness(X, X_emb, n_neighbors=i)

	return ks, trust





def NNP(X, X_emb, Khigh=30):
	neigh_high = NearestNeighbors(n_neighbors=Khigh, n_jobs=-1)
	neigh_high.fit(X)
	high_dists, high_indices = neigh_high.kneighbors(X)

	
	neigh_emb = NearestNeighbors(n_neighbors=Khigh, n_jobs=-1)
	neigh_emb.fit(X_emb)
	emb_dists, emb_indices = neigh_emb.kneighbors(X_emb)	

	m_precision = np.zeros(Khigh)
	m_recall = np.zeros(Khigh)
	for k in range(Khigh):
		tp = np.zeros(X.shape[0])
		for i in range(X.shape[0]):
			current = emb_indices[i][:k+1]
			tp[i] = len(np.intersect1d(high_indices[i], current))
		precision = tp/float(k+1)
		recall = tp/Khigh

		m_precision[k] = precision.mean()
		m_recall[k] = recall.mean()

	return m_precision, m_recall


def get_size(value):

	area = (math.PI/4.0)*1



level = 0

n = 5000
n_neighbors = 15


fashionTrain = pd.read_csv('data/fashion-train.csv')

fashionX = fashionTrain.values[:,2:]
fashionY = fashionTrain.values[:, 1].astype(int)

print(fashionX.shape, fashionY.shape)

X = fashionX
y = fashionY
X = check_array(X, dtype=np.float32, accept_sparse="csr", order="C")
# X = np.load('./data/MNIST_70000.npy')
# y = np.load('./data/MNIST_70000_label.npy').astype(int)
# X = normalize(X)
# print(greatest.shape, cols.shape, graph.shape, knn_dists.shape)

print(X.shape)

# cpp_umap = h_umap.UMAP("euclidean", "FAISS_IVFFlat")
# start = time.time()
# cpp_umap.fit_hierarchy(mnist)

# end = time.time()
# print("time: %.5fs" % (end-start))

hUmap = h_umap.HUMAP("precomputed", np.array([0.22, 0.19]), 15, "KDTree_NNDescent", True)
hUmap.fit(X, y)


second_level = X[hUmap.get_indices(0),:]
third_level = second_level[hUmap.get_indices(1),:]




y2 = hUmap.get_labels(2)
embedding2 = hUmap.get_embedding(2)


# demap_level3 = demap.DEMaP(third_level, embedding2)
# print(demap_level3)



y1 = hUmap.get_labels(1)
embedding1 = hUmap.get_embedding(1)


# indices1 = hUmap.get_indices(1)
# plt.scatter(embedding1[indices1, 0], embedding1[indices1, 1], c ='red', alpha=1, s=1)

# precision2, recall2 = NNP(third_level, embedding2)
# precision, recall = NNP(second_level, embedding1)

# precision_third, recall_third = compute_trustworthiness(third_level, embedding2)
# precision_second, recall_second = compute_trustworthiness(second_level, embedding1)


# plt.plot(precision2, recall2)
# plt.show()


# plt.plot(precision, recall)
# plt.show()


influence2 = hUmap.get_influence(2)
influence1 = hUmap.get_influence(1)
maxValue = max(np.max(influence1), np.max(influence2))
print(np.max(influence2), np.max(influence1))
print(influence2)
print("influence2")
print(np.sum(influence2), np.sum(influence2==0))
# for i in range(len(influence2)):
# 	if influence2[i] == 0:
# 		print(i, " >> ", influence2[i])
print("\n\ninfluence1")
print(np.sum(influence1), np.sum(influence1==0))
# for i in range(len(influence1)):
# 	if influence1[i] == 0:
# 		print(i, " >> ", influence1[i])
	

# plt.plot(precision_third, recall_third)
# plt.ylim(0, 1)
# plt.show()

# plt.plot(precision_second, recall_second)
# plt.ylim(0, 1)
# plt.show()
s2 = transform_sizes(influence2, 1, maxValue, rightMin=8, rightMax=100)
s1 = transform_sizes(influence1, 1, maxValue, rightMin=8, rightMax=100)
s0 = transform_sizes([1]*len(y), 1, maxValue, rightMin=8, rightMax=100)

print("s2")
print(s2[:10])

print("\ns1")
print(s1[:10])

print("\ns0")
print(s0[:10])


plt.scatter(embedding2[:, 0], embedding2[:, 1], c = y2, cmap='Spectral', alpha=0.7, s=influence2*3)
plt.show()
plt.scatter(embedding1[:, 0], embedding1[:, 1], c = y1, cmap='Spectral', alpha=0.7, s=influence1*3)
plt.show()




embedding0 = hUmap.get_embedding(0)
plt.scatter(embedding0[:, 0], embedding0[:, 1], c = y, cmap='Spectral', alpha=0.7, s=1)
indices0 = hUmap.get_indices(0)
# plt.scatter(embedding0[indices0, 0], embedding0[indices0, 1], c ='red', alpha=1, s=1)
plt.show()


print("Num points in scale %d: %d" % (2, len(embedding2)))
print("Num points in scale %d: %d" % (1, len(embedding1)))
print("Num points in scale %d: %d" % (0, len(embedding0)))

# sigmas0 = hUmap.get_sigmas(0)
# sigmas0 = np.sort(sigmas0)
# print("sigmas0: ")
# print(sigmas0[:10])
# df0 = pd.DataFrame({
# 	'sigmas': sigmas0
# 	})
# #ax = df0.plot.hist(bins=20, alpha=0.5)
# ax = sns.distplot(sigmas0)
# plt.show()


# sigmas1 = hUmap.get_sigmas(1)
# sigmas1 = np.sort(sigmas1)
# print("sigmas1: ")
# print(sigmas1[:10])
# df1 = pd.DataFrame({
# 	'sigmas': sigmas1
# 	})
# #ax = df1.plot.hist(bins=20, alpha=0.5)
# ax = sns.distplot(sigmas1)
# plt.show()










# cpp_umap = humap.UMAP("precomputed")
# start = time.time()
# cpp_umap.fit_hierarchy_sparse(data)
# end = time.time()
# print("time: %.5fs" % (end-start))
