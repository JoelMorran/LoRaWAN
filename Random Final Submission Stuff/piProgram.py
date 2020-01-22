# -*- coding: utf-8 -*-
"""

@author: 18938149

"""

#for adding several features to the data
def add_feature(data):
    data["x_y"]=(data.Axis1*data.Axis2) 
    data["y_z"]=(data.Axis2*data.Axis3)
    data["x_z"]=(data.Axis1*data.Axis3)
    data["Multiply"]=((data.Axis1*data.Axis1)+(data.Axis2*data.Axis2)+(data.Axis3*data.Axis3))
    data["Mag_Vec_Feature"] = np.sqrt(data.Multiply) 
    return data 

#for preprocessing the test data    
def test_preprocess(time_steps,step):
    # Number of steps to advance in each iteration (for me, it should always 
    # be equal to the time_steps in order to have no overlap between segments)
    # step = time_steps
    segments = []
    for i in range(0, len(data) - time_steps, step):
        xs = data['Axis1'].values[i: i + time_steps]
        ys = data['Axis2'].values[i: i + time_steps]
        zs = data['Axis3'].values[i: i + time_steps]
        x_y = data['x_y'].values[i: i + time_steps]
        y_z = data['y_z'].values[i: i + time_steps]
        x_z = data['x_z'].values[i: i + time_steps]
        Mag= data['Mag_Vec_Feature'].values[i: i + time_steps]
        segments.append([xs, ys, zs, Mag, x_y, y_z, x_z])
    return segments

#for inflating prediction data to conform with dataframe        
def label_output(max_y_pred_test, step):
    y_pred = []
    for i in range(0, len(max_y_pred_test), step):
        for j in range(0, 30, 1):
            y_pred.append(max_y_pred_test[i])
    return y_pred

#if the data that is imported is an irregular size than expected, trims the
#dataframe so that the prediction data column can be added
def dataframe_trim(data, step_size):
    resultant = len(data)/step_size
    dec, int_ = math.modf(resultant)
    n = round(dec,2) * step_size
    data.drop(data.tail(int(n)).index,inplace=True)
     
#%%
import numpy as np   
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import math
import urllib

from sqlalchemy import create_engine

from sklearn import metrics
from sklearn.metrics import classification_report 
import pickle    
import scipy as sc

from keras.models import Sequential, load_model
from keras.layers import Dense, Dropout, LSTM, Flatten, core, Reshape
from keras.layers.convolutional import Conv1D
from keras.layers.convolutional import MaxPooling1D
from keras.callbacks import ModelCheckpoint
from keras.models import model_from_json
from keras.optimizers import SGD
from keras.callbacks import EarlyStopping
from keras.wrappers.scikit_learn import KerasClassifier
from keras.models import Sequential
from keras.layers import Conv2D, MaxPooling2D,GlobalAveragePooling1D
from keras.utils import np_utils,to_categorical
from keras.models import load_model

#%%Reading data from mssql database
#creating connection to database
engine = create_engine('mssql+pyodbc:///?odbc_connect=' + 
                       urllib.parse.quote_plus('DRIVER=FreeTDS;SERVER=teamy-server.database.windows.net;PORT=1433;DATABASE=teamy_db;UID=teamy;PWD=Latrobe1234;TDS_Version=8.0;')
)

#querying the databse to pull data
data = pd.read_sql('SELECT * FROM TeamF', engine)

#data = pd.read_csv("Train_Data_Break.csv")

#%%Reading the label encoder file written in Train file
pkl_file = open('encoder.pkl', 'rb')
le_new = pickle.load(pkl_file) 

#%%Adding the fetures
data=add_feature(data)

#%%Encoding the class labels
LABELS=list(le_new.classes_)
print(list(le_new.classes_)) 

#%%preprocessing the test data
#test_preprocess(time_steps,step),advisable to go with value 30 for both step and time_steps
test=test_preprocess(30,30)

#%%reshaping the test data
test=np.array(test)

test=np.transpose(test,(0,2,1))

#%%running predictiong on data with the model
model = load_model('ML_Model.h5')
y_pred_test = model.predict(test)
max_y_pred_test = np.argmax(y_pred_test, axis=1)

#%%reshaping dataframe to insert prediction column
dataframe_trim(data, 30)
expandPred = label_output(max_y_pred_test, 1)
data['Activity'] = expandPred
data["Activity"] = le_new.inverse_transform(data["Activity"].values.ravel())

#dropping unnecessary columns and uploading to mssql database
data.drop(["Axis1", "Axis2", "Axis3", "x_y", "y_z", "x_z", "Multiply", "Mag_Vec_Feature"], axis=1, inplace=True)
data.to_sql('TeamF_Final', engine, if_exists='replace', index=False)

#%%