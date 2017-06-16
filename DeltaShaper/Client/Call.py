#!python
import sys, os, signal
import time, threading
import subprocess as sub
import Skype4Py


CallStatus = 0  #To be updated on OnCall
CallRef = 0
ClientProc = 0
ServerProc = 0
CallIsFinished = set ([Skype4Py.clsFailed, Skype4Py.clsFinished, Skype4Py.clsMissed, Skype4Py.clsRefused, Skype4Py.clsBusy, Skype4Py.clsCancelled]);
TimerInterval = int(5)

def signal_handler(signal, frame):
        print "Exiting Python Wrapper..."
        os.system("sudo kill -s SIGINT %d"%(ClientProc.pid))
        os.system("sudo kill %d"%(ServerProc.pid))
        CallRef.Finish()
        time.sleep(1)
        os.system("reset")
        sys.exit(0)

class CallSampler:

    def AttachmentStatusText(self, status):
       return self.skype.Convert.AttachmentStatusToText(status)

    def CallStatusText(self, status):
        return self.skype.Convert.CallStatusToText(status)

    # This handler is fired when status of Call object has changed
    def OnCall(self, call, status):
        global CallRef
        CallRef = call
        print "status is ", status
        if status == Skype4Py.clsInProgress:
            print "length of active calls is: ",len(self.skype.ActiveCalls)
            #StartVideoSend performed through Skype GUI option
            args = ['sudo', '../Server/vp', '-r', '1', '-p', '250000']
            proc = sub.Popen(args)
            global ServerProc
            ServerProc = proc

    # This handler is fired when Skype attachment status changes
    def OnAttach(self, status): 
        if status == Skype4Py.apiAttachAvailable:
            skype.Attach()
        

    def start(self):
            self.skype = Skype4Py.Skype()
            self.skype.OnAttachmentStatus = self.OnAttach
            self.skype.OnCallStatus = self.OnCall

    def Attach(self):
        self.skype.Attach()

    def Callout(self, callee):
        Found = False
        for friend in sampler.skype.Friends:
            if friend.Handle == contactHandle:
                Found = True
                self.skype.PlaceCall(callee)
                break
        if not Found:
            print 'Callee is not in contact list'
            sys.exit()


if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal_handler)

    sampler = CallSampler()
    sampler.start()
    sampler.Attach()

    try:
        contactHandle = sys.argv[1]
    except:
        print 'Missing callee handle'
        sys.exit()

    args = ['sudo', './div', '-r', '1', '-f', '1']
    proc = sub.Popen(args)
    global ClientProc
    ClientProc = proc
    time.sleep(10)
    try:
        sampler.Callout(contactHandle) #place call after setting up pipeline
    except:
        os.system("sudo kill -s SIGINT %d"%(ClientProc.pid))


    while not CallStatus in CallIsFinished:
            end = raw_input("Ctrl-C to finish: ")
