#!/usr/bin/env python

import dpkt
import subprocess
import socket
import os
import math
from itertools import product
import numpy as np
from matplotlib import pyplot as plt
from matplotlib.pyplot import cm
from pyemd import emd
import collections
import fcntl
import struct

BIN_WIDTH = 50

def binData(data):

    bindata = []
    for i in data:
        bindata.append(RoundToNearest(i))
    
    return bindata

def RoundToNearest(n, m=BIN_WIDTH):
        r = n % m
        return n + m - r if r + r >= m else n - r


def GatherChatSamples(evaluation, param):
    Samples = []
    folder = './../Client/ReferenceStreams/'
    for cap in os.listdir(folder):
        if (cap.endswith(evaluation + '_' + str(param)+ '_30_0')):
                Samples.append(folder + cap + '/packetCount_' + str(BIN_WIDTH))
    return Samples


def Classifier(toClassify, baseSamples):
    emdResults = []
    emdSum = 0

    ##################################
    #Read first element in combination
    ##################################
    f = open(toClassify, 'r')

    core_Gk = {}
    bins=[]
    #Generate the set of all possible bins
    for i in range(0,1500, BIN_WIDTH):
        core_Gk[str(i).replace(" ", "")] = 0
        
        
    lines = f.readlines()
    for line in lines:
        try:
            bins.append(line.rstrip('\n'))
        except IndexError:
            break #Reached last index, stop processing
    f.close()
    #Account for each bin elem
    for i in bins:
        core_Gk[str(i)]+=1

    od_core = collections.OrderedDict(sorted(core_Gk.items()))   
    Gk_corelist = []
    for i in od_core:
        Gk_corelist.append(float(od_core[i]))
    Gk_corelist = np.array(Gk_corelist)
        
    for sample in baseSamples:  
        f = open(sample, 'r')

        Gk = {}
        bins=[]
        #Generate the set of all possible bins
        for i in range(0,1500, BIN_WIDTH):
            Gk[str(i).replace(" ", "")] = 0
            
            
        lines = f.readlines()
        for line in lines:
            try:
                bins.append(line.rstrip('\n'))
            except IndexError:
                break #Reached last index, stop processing

        #Account for each bin elem
        for i in bins:
            Gk[str(i)]+=1

        od = collections.OrderedDict(sorted(Gk.items())) 
        Gklist = []
        for i in od:
            Gklist.append(float(od[i]))
        Gklist = np.array(Gklist)
        
        distance_matrix = []
        for i in range(0,len(Gk)):
            line =[]
            for j in range(0,len(Gk)):
                if(i==j):
                    line.append(0.0)
                else:
                    line.append(1.0)

            distance_matrix.append(np.array(line))
        distance_matrix = np.array(distance_matrix)
        
        ############################
        ###### NORMALIZATION #######
        ############################
        ground1 = max(Gk_corelist)
        ground2 = max(Gklist)
        if(ground1 > ground2):
            MAX = ground1
        else:
            MAX = ground2
        
        if(max(np.cumsum(Gk_corelist)) > max(np.cumsum(Gklist))):   
            cSum = max(np.cumsum(Gk_corelist))
        else:
            cSum =  max(np.cumsum(Gklist))

        Gk_corelist2 = Gk_corelist#/MAX
        Gklist = Gklist#/MAX
        distance_matrix = distance_matrix/cSum


        emdR = float(emd(Gk_corelist2, Gklist, distance_matrix))

        emdSum += emdR
        emdResults.append(emd(Gk_corelist2, Gklist, distance_matrix))
        f.close()

    avgEMD = emdSum / len(emdResults)
    dot, duodot, client, cal, capFile = toClassify.split('/')
    return [capFile, avgEMD]


def get_ip_address(ifname):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    return socket.inet_ntoa(fcntl.ioctl(
        s.fileno(),
        0x8915,  # SIOCGIFADDR
        struct.pack('256s', ifname[:15])
    )[20:24])

def ParseCapture():
    if not os.path.exists('./../Client/CalibrationFiltered'):
                os.makedirs('./../Client/CalibrationFiltered')

    machineIP = get_ip_address('eth0') 


    # Parse Selectors traffic captures
    for cap in os.listdir('./../Client/CalibrationRaw'):
        packet_count = open('./../Client/CalibrationFiltered/' + cap, 'w')
        f = open('./../Client/CalibrationRaw/' + cap)
        pcap = dpkt.pcap.Reader(f)


        for ts, buf in pcap:
            eth = dpkt.ethernet.Ethernet(buf)
            ip_hdr = eth.data
            try: #TODO Ugly hardcode with these srcPorts, these are the ones we use. These should be adjust by configuration
                src_ip_addr_str = socket.inet_ntoa(ip_hdr.src)
                if ((ip_hdr.data.sport == 36128 or ip_hdr.data.sport == 24986) and src_ip_addr_str == machineIP):
                    packet_count.write("%s\n" % RoundToNearest(len(buf)))
            except:
                pass #destination may not be translatable by inet_ntoa

        packet_count.close()
        f.close()


    #Parse Baseline traffic capture
    packet_count = open('./../Client/CalibrationBaselines/base.packet_count', 'w')
    f = open('./../Client/CalibrationBaselines/base.pcap', 'r')
    pcap = dpkt.pcap.Reader(f)


    for ts, buf in pcap:
        eth = dpkt.ethernet.Ethernet(buf)
        ip_hdr = eth.data
        try: #TODO Ugly hardcode with these srcPorts, these are the ones we use. These should be adjust by configuration
            src_ip_addr_str = socket.inet_ntoa(ip_hdr.src)
            if ((ip_hdr.data.sport == 36128 or ip_hdr.data.sport == 24986) and src_ip_addr_str == machineIP):
                packet_count.write("%s\n" % RoundToNearest(len(buf)))
        except:
            pass #destination may not be translatable by inet_ntoa

    packet_count.close()
    f.close()

if __name__ == "__main__":

    #Regular chat samples to compare with. Here we are just picking those with no perturbations.
    #We can either pick other baseline to compare with or mix exploration of perturbed samples.
    baseSamples = GatherChatSamples("bandwidth", 0)
    ParseCapture()

    streams = []


    for cap in os.listdir('./../Client/CalibrationFiltered/'):
        streams.append('./../Client/CalibrationFiltered/' + cap)

    selectors = []
    for stream in streams:
        selectors.append(Classifier(stream,baseSamples))

    selectors = sorted(selectors, key=lambda x: x[1])
    for selector in selectors:
        print str(selector[0]) + " " + str(selector[1])
