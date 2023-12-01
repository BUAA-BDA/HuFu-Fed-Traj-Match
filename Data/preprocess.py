# code for trajectory data preprocessing

import os
from datetime import timedelta
import pandas as pd
import numpy as np

def filter_out_of_bound(df_list,bound):
    res_list = []
    for df in df_list:
        if df['x'].min() >= bound[0][0] and df['x'].max() <= bound[0][1] and df['y'].min() >= bound[1][0] and df['y'].max() <= bound[1][1]:
            res_list.append(df)
    return res_list

def filter_by_velocity(df_list, max_v):
    res_list = []
    # in seconds
    resample_interval = (df_list[0].iloc[1]['time'] - df_list[0].iloc[0]['time']).seconds
    for df in df_list:
        # calculate the velocity of each point
        df['distance'] = np.sqrt((df['x'] - df['x'].shift(1))**2 + (df['y'] - df['y'].shift(1))**2)
        df['velocity'] = df['distance'] / resample_interval
        if df['velocity'].max() <= max_v:
            res_list.append(df[['lat','lon','time','x','y']])
    return res_list

def filter_short_traj(df_list, min_length):
    res_list = []
    for df in df_list:
        if df.shape[0] >= min_length:
            res_list.append(df)
    return res_list

def geo2cartesian(startPos, endPos):
    """ 
    Creates new columns with x, y coordinates in metres (converted from the latitude and longitude coordinates),
    fix start point as [1,-1], end point as [-1,-1]
    input:
        startPos: the origin point of the cartesian coordinate system, [lat, lon]
        endPos: the target point of the cartesian coordinate system, [lat, lon]
    output:
        [x,y]: the coordinates of the target point in the cartesian coordinate system
    """
    
    
    def haversine(startPos, endPos):
        """
        Calculate the great circle distance between two points
        on the earth (specified in decimal degrees)

        All args must be of equal length.    

        """

        radius = earthRad(endPos[0])

        lon1, lat1, lon2, lat2 = map(np.radians, [startPos[1], startPos[0], endPos[1], endPos[0]])

        dlon = lon2 - lon1
        dlat = lat2 - lat1

        a = np.sin(dlat/2.0)**2 + np.cos(lat1) * np.cos(lat2) * np.sin(dlon/2.0)**2
        c = 2 * np.arcsin(np.sqrt(a))
        distance = radius * c
        return distance

    def gimmeCoords(origin, endPos):
        '''
        Uses haversine to map the lat/long coords to (pseudo-)cartesian coordinates in metres with a specified origin

        '''
        x = haversine(origin, [origin[0], endPos[1]])
        y = haversine(origin, [endPos[0], origin[1]])

        if endPos[1] - origin[1] < 0:
            x = x*(-1)
        if endPos[0] - origin[0] < 0:
            y = y*(-1)

        return [x,y]

    def earthRad(Lat):
        '''
        Calculates the Earth's radius (in m) at a given latitude using an ellipsoidal model. Major/minor axes from NASA

        '''
        a = 6378137
        b = 6356752
        Lat = np.radians(Lat)
        g = (a**2*np.cos(Lat))**2 + (b**2*np.sin(Lat))**2
        f = (a*np.cos(Lat))**2 + (b*np.sin(Lat))**2
        R = np.sqrt(g/f)
        return R
    
    return gimmeCoords(startPos, endPos)


def read_dataset2df(path,interval,sample_num = None):
    df_list = []
    cur_sample_num = 0
    for root, dirs, files in os.walk(path):
        for dir in dirs:
            if dir.isdigit():
                trajectory_path = os.path.join(root, dir, 'Trajectory')
                for file in os.listdir(trajectory_path):
                    # get the file path of each user's traejcetory file
                    if file.endswith('.plt'):
                        file_path = os.path.join(trajectory_path, file)
                        df = pd.read_csv(file_path, skiprows=6, header=None,index_col= False,names=['lat', 'lon', 'zero', 'alt','data_str' ,'date', 'time'])
                        df = df[['lat', 'lon', 'time']]
                        df['time'] = pd.to_datetime(df['time'], format='%H:%M:%S')
                        df_list.append(df)
                        cur_sample_num += 1
                        if sample_num and cur_sample_num >= sample_num:
                            return df_list
                        
    return df_list

def save_processed_traj(df_list, path, decimal=2):
    with open(path, 'w') as f:
        for df in df_list:
            for i in range(len(df)):
                f.write(str(round(df.iloc[i]['x'],decimal)) + ',' + str(round(df.iloc[i]['y'],decimal)) + ',' + str(df.iloc[i]['time'].strftime('%H:%M:%S')) + ';')
            f.write('\n')
    f.close()
    

                        
def process_dataset(path, origin_pos ,interval, bound, sample_num = None):
    max_v = 100
    
    df_list = read_dataset2df(path,interval,sample_num)
    print(len(df_list))
    i = 0
    for df in df_list:
        if i % 100 == 0:
            print(i)
        i += 1
        df[['x','y']] = df.apply(lambda x: geo2cartesian(origin_pos, [x['lat'], x['lon']]), axis=1, result_type='expand')

    # Filtering
    df_list = filter_short_traj(df_list, 20)
    df_list = filter_out_of_bound(df_list, bound)
    df_list = filter_by_velocity(df_list, max_v)
    return df_list

if __name__ == '__main__':
    raw_data_path = '...'
    processed_data_path = '...'
    origin_pos = [0, 0]
    # the bound of the dataset, in metres
    bound = [[0, 0],[0, 0]]
    
    interval = 60
    df_list = process_dataset(raw_data_path, origin_pos, interval, bound)
    save_processed_traj(df_list, processed_data_path)