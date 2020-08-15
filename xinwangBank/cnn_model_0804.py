import numpy as np
import pandas as pd
import random
from tqdm import tqdm
from scipy.signal import resample
import tensorflow as tf
from tensorflow import keras
from functools import partial
from tensorflow.keras.layers import *
from tensorflow.keras.models import Model
from tensorflow.keras.optimizers import Adam
from tensorflow.keras.utils import to_categorical
from sklearn.model_selection import StratifiedKFold
from tensorflow.keras.callbacks import EarlyStopping, ModelCheckpoint, ReduceLROnPlateau


# 加载数据
train_1 = pd.read_csv('data/sensor_train.csv')
train_2 = pd.read_csv('data/pseudo_train_2756.csv')
train = pd.concat([train_1, train_2])
train_fragment_id = list(train['fragment_id'].drop_duplicates())

test = pd.read_csv('data/sensor_test.csv')
sub = pd.read_csv('data/submit.csv')
y = train.groupby('fragment_id')['behavior_id'].min()

#构建特征
train['mod'] = (train.acc_x ** 2 + train.acc_y ** 2 + train.acc_z ** 2) ** .5
train['modg'] = (train.acc_xg ** 2 + train.acc_yg ** 2 + train.acc_zg ** 2) ** .5
test['mod'] = (test.acc_x ** 2 + test.acc_y ** 2 + test.acc_z ** 2) ** .5
test['modg'] = (test.acc_xg ** 2 + test.acc_yg ** 2 + test.acc_zg ** 2) ** .5

# 采样
x = np.zeros((len(train_fragment_id), 60, 8))
t = np.zeros((7500, 60, 8))
for i in tqdm(range(len(train_fragment_id))):
    tmp = train[train.fragment_id == train_fragment_id[i]][:60]
    x[i,:,:] = resample(tmp.drop(['fragment_id', 'time_point', 'behavior_id'],
                                    axis=1), 60, np.array(tmp.time_point))[0]

for i in tqdm(range(7500)):
    tmp = test[test.fragment_id == i][:60]
    t[i,:,:] = resample(tmp.drop(['fragment_id', 'time_point'],
                                    axis=1), 60, np.array(tmp.time_point))[0]


# 数据增强
def shuffle_index_fun(index):
    """shuffle作用就是重新排序返回一个随机序列作用类似洗牌"""
    np.random.shuffle(index)
    return index

def shuffle_data(data_x, data_y):
    """
    数据进行洗牌
    """
    shuffle_index = list(range(data_x.shape[0]))
    r_index = shuffle_index_fun(shuffle_index)

    return data_x[r_index], data_y[r_index]

def mixup(batch_x, batch_y, random_max=0.02):
    """
    数据索引进行洗牌混合
    """
    shuffle_index = list(range(batch_x.shape[0]))
    r_index1 = shuffle_index_fun(shuffle_index)
    r_index2 = shuffle_index_fun(shuffle_index)

    rd = random.uniform(0, random_max)#将随机生成下一个实数，它在 [0,0.2] 范围内。
    batch_x_mixup = (1-rd) * batch_x[r_index1] + rd * batch_x[r_index2]

    batch_y_mixup = (1-rd) * batch_y[r_index1] + rd * batch_y[r_index2]

    return batch_x_mixup, batch_y_mixup

def noise(batch_x, batch_y, random_max=0.02):

    size = batch_x.shape
    
    batch_x_noise = batch_x + np.random.uniform(-random_max,random_max,size=size)
    batch_y_noise = batch_y

    return batch_x_noise, batch_y_noise

def generator(data_x, data_y):

    list_batch_x, list_batch_y = [], []

    list_batch_x.append(data_x)
    list_batch_y.append(data_y) 

    batch_x, batch_y = shuffle_data(data_x, data_y)
    batch_x_mixup, batch_y_mixup = mixup(batch_x, batch_y, random_max=0.05)
    list_batch_x.append(batch_x_mixup), list_batch_y.append(batch_y_mixup)

    batch_x_noise, batch_y_noise = noise(batch_x, batch_y, random_max=0.01)
    list_batch_x.append(batch_x_noise), list_batch_y.append(batch_y_noise)

    batch_x_yield, batch_y_yield = np.vstack(list_batch_x), np.vstack(list_batch_y)
    x, y = shuffle_data(batch_x_yield, batch_y_yield)

    return x, y

# 自定义准确率
def acc_combo(y_true, y_pred):
    
    try:
        map_scene = tf.constant(['A','A','A','A','D','A','B','B','B','B','B','A','C','C','C','B','C','C','C'])
        map_action = tf.constant(['0','1','2','3','4','5','1','5','2','3','0','6','1','3','0','6','2','5','6'])

        y_true_index, y_pred_index = tf.argmax(y_true,axis=1), tf.argmax(y_pred,axis=1)
        y_true_index_scene, y_pred_index_scene = tf.nn.embedding_lookup(map_scene,y_true_index), tf.nn.embedding_lookup(map_action,y_pred_index)
        y_true_index_action, y_pred_index_action = tf.nn.embedding_lookup(map_action,y_true_index), tf.nn.embedding_lookup(map_action,y_pred_index)

        acc_scene = tf.cast(tf.equal(y_true_index_scene, y_pred_index_scene),tf.float16) 
        acc_action = tf.cast(tf.equal(y_true_index_action, y_pred_index_action),tf.float16)
        acc_classification = tf.cast(tf.equal(y_true_index, y_pred_index),tf.float16)
        score = tf.reduce_mean(1.0/7 * acc_scene + 1.0/3 * acc_action + 11/21 * acc_classification)

    except:
        print(y_true), print(y_pred)
        print(y_true_index), print(y_pred_index)

    return score 

# 网络结构1
def Convnet(input_shape=(60,8,1), classes=19):
    
    input = Input(shape=(60, 8, 1))
    X = Conv2D(filters=64,
               kernel_size=(3, 3),
               activation='relu',
               padding='same')(input)
    X = Dropout(0.3)(X) 
    X = Conv2D(filters=128,
               kernel_size=(3, 3),
               activation='relu',
               padding='same')(X)
    X = MaxPooling2D()(X)
    X = Dropout(0.3)(X)

    X = Conv2D(filters=256,
               kernel_size=(3, 3),
               activation='relu',
               padding='same')(X)
    X = Dropout(0.3)(X)
    X = Conv2D(filters=512,
               kernel_size=(3, 3),
               activation='relu',
               padding='same')(X)
    X = GlobalMaxPooling2D()(X)
    X = Dropout(0.5)(X)

    X = Dense(256, activation='relu')(X)
    X = Dropout(0.5)(X)
    X = Dense(128, activation='relu')(X)
    X = Dropout(0.5)(X)
    X = Dense(19, activation='softmax')(X)

    return Model([input], X)

# 网络结构2
def BLOCK(seq, filters, kernal_size):

    cnn = keras.layers.Conv1D(filters, 1, padding='SAME', activation='relu')(seq)
    cnn = keras.layers.LayerNormalization()(cnn)

    cnn = keras.layers.Conv1D(filters, kernal_size, padding='SAME', activation='relu')(cnn)
    cnn = keras.layers.LayerNormalization()(cnn)

    cnn = keras.layers.Conv1D(filters, 1, padding='SAME', activation='relu')(cnn)
    cnn = keras.layers.LayerNormalization()(cnn)

    seq = keras.layers.Conv1D(filters, 1)(seq)
    seq = keras.layers.Add()([seq, cnn])

    return seq

def BLOCK2(seq, filters=128, kernal_size=5):

    seq = BLOCK(seq, filters, kernal_size)
    seq = keras.layers.MaxPooling1D(2)(seq)
    seq = keras.layers.SpatialDropout1D(0.3)(seq)
    seq = BLOCK(seq, filters//2, kernal_size)
    seq = keras.layers.GlobalAveragePooling1D()(seq)

    return seq

def ComplexConv1D(input_shape=(60,8), num_classes=19):

    inputs = keras.layers.Input(shape=input_shape)
    seq_3 = BLOCK2(inputs, kernal_size=3)
    seq_5 = BLOCK2(inputs, kernal_size=5)
    seq_7 = BLOCK2(inputs, kernal_size=7)
    seq = keras.layers.concatenate([seq_3, seq_5, seq_7])
    seq = keras.layers.Dense(512, activation='relu')(seq)
    seq = keras.layers.Dropout(0.3)(seq)
    seq = keras.layers.Dense(128, activation='relu')(seq)
    seq = keras.layers.Dropout(0.3)(seq)
    outputs = keras.layers.Dense(num_classes, activation='softmax')(seq)
    model = keras.models.Model(inputs=[inputs], outputs=[outputs])

    return model

# 模型训练
kfold = StratifiedKFold(5,random_state=2020,shuffle=True)
proba_t = np.zeros((7500, 19))

for fold, (xx, yy) in enumerate(kfold.split(x, y)):
    
    y_ = to_categorical(y, num_classes=19)
    x_ge, y_ge = generator(x[xx], y_[xx])

    model = ComplexConv1D()
    model.compile(loss='categorical_crossentropy', # 此处可以自定义loss, loss = my_loss
                  optimizer=Adam(),
                  metrics=[acc_combo, 'acc'])
    plateau = ReduceLROnPlateau(monitor="val_acc",
                                verbose=1,
                                mode='max',
                                factor=0.5,
                                patience=8)
    early_stopping = EarlyStopping(monitor='val_acc',
                                   verbose=0,
                                   mode='max',
                                   patience=18)
    checkpoint = ModelCheckpoint(f'CNN_baseline_0804/fold{fold}.h5',
                                 monitor='val_acc',
                                 verbose=0,
                                 mode='max',
                                 save_best_only=True)
    model.fit(x_ge, y_ge,
              epochs=500,
              batch_size=256,
              verbose=1,
              shuffle=True,
              validation_data=(x[yy], y_[yy]),
              callbacks=[plateau, early_stopping, checkpoint])
              
    model.load_weights(f'CNN_baseline_0804/fold{fold}.h5')
    proba_t += model.predict(t, batch_size=32)/5


# 输出结果
sub.behavior_id = np.argmax(proba_t, axis=1)
sub.to_csv('CNN_baseline_0804/submit.csv', index=False)