#!python
'''
@author: John Chen (johnsofteng at gmail.com) 
Modified by Diogo Barradas
'''

import sys, time, socket
import Skype4Py
import os, re, signal
import subprocess as sub
import time, threading

ServerProc = 0
ClientProc = 0

def signal_handler(signal, frame):
        print "Exiting Python Wrapper..."
        os.system("sudo kill -s SIGINT %d"%(ClientProc.pid))
        os.system("sudo kill %d"%(ServerProc.pid))
	time.sleep(1)
	os.system("reset")
        sys.exit(0)

class receive_set:
    def __init__(self):
        self.answered = False

    def OnCall(self, call, status):
        print "status is ", status, " Peer is: ", call.PartnerHandle, " Show name is ", call.PartnerDisplayName
        print "length of active calls are ",len(self.skype.ActiveCalls)
        inprogress = False
        if (status == Skype4Py.clsRinging) and (call.Type == Skype4Py.cltIncomingP2P or call.Type == Skype4Py.cltIncomingPSTN):
            for curr in self.skype.ActiveCalls:
                print "Call status is ", curr.Type, " status is ", curr.Status
                if curr.Status == Skype4Py.clsInProgress :
                    inprogress = True
            if not inprogress:
                if(not self.answered):
                    call.Answer()
                    self.answered = True
                
        if (status == Skype4Py.clsInProgress):
            args = ['sudo', './vp', '-r', '0', '-p', '250000']
            proc = sub.Popen(args)
            global ServerProc
            ServerProc = proc

        if (status == Skype4Py.clsFinished or status == Skype4Py.clsCancelled):
            self.answered = False
            print "Call ended. Shutting down server..."
            os.system("sudo kill %d"%(ServerProc.pid))
            

    def OnAttach(self, status):
        print 'API attachment status:'+self.skype.Convert.AttachmentStatusToText(status)
        if status == Skype4Py.apiAttachAvailable:
            self.skype.Attach()
            
    def start(self):
        self.skype = Skype4Py.Skype()
        self.skype.OnAttachmentStatus = self.OnAttach
        self.skype.OnCallStatus = self.OnCall


    def Attach(self):
        self.skype.Attach()


if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal_handler)

    args = ['sudo', '../Client/div', '-r', '0', '-f', '1']
    proc = sub.Popen(args)
    global ClientProc
    ClientProc = proc
    time.sleep(10)

    rec = receive_set()
    rec.start()
    rec.Attach()

    while 1:
        time.sleep(1)
