"""Procura os ajustes do DrikSynth que mais aproximam um trecho de disco.

Uso:  python3 ajustar_preset.py   (com trecho.wav, estereo 44,1 kHz, na pasta atual)

O que e medido no trecho e em cada tentativa: energia por oitava, nivel dos harmonicos da nota
mais grave, densidade do espectro entre 1 e 8 kHz, oscilacao de altura dos harmonicos e
semelhanca entre os canais. As notas do acorde e a grade de busca estao no fim do arquivo e
precisam ser adaptadas a cada trecho. Requer o DrikRender compilado (cmake -DDRIK_BUILD_TESTS=ON).
"""
import numpy as np, subprocess, itertools, json
RENDER = "build/DrikRender_artefacts/Release/DrikRender"
from scipy.io import wavfile
from scipy import signal
H=[1,2,3,4,5,6,8,12,16,24]
def feat(path,t0,t1):
    sr,x=wavfile.read(path); x=x.astype(float)/32768; x=x[int(t0*sr):int(t1*sr)]
    L,R=x[:,0],x[:,1]; m=(L+R)/2
    f,P=signal.welch(m,sr,nperseg=16384,noverlap=12288)
    band=(f>100)&(f<120); f0=f[band][P[band].argmax()]
    hs=np.array([10*np.log10(max(P[np.abs(f-f0*h)<6])/P.max()) for h in H])
    octs=np.array([10*np.log10(P[(f>=lo)&(f<2*lo)].sum()/P.sum()+1e-20) for lo in [62,125,250,500,1000,2000,4000,8000]])
    b=(f>1000)&(f<8000); flat=np.log10(np.exp(np.mean(np.log(P[b]+1e-20)))/np.mean(P[b]))
    devs=[]
    for fc in [f0*2,f0*3,f0*6,f0*8]:
        sos=signal.butter(4,[fc-25,fc+25],btype='band',fs=sr,output='sos'); y=signal.sosfiltfilt(sos,m); a=signal.hilbert(y)
        fi=np.diff(np.unwrap(np.angle(a)))*sr/(2*np.pi); k=int(.1*sr); fi=fi[k:-k]
        bb=signal.firwin(int(sr*0.02)|1,40,fs=sr); c=1200*np.log2(np.clip(signal.filtfilt(bb,[1],fi),1,None)/fc)
        devs.append((np.percentile(c,95)-np.percentile(c,5))/2)
    return dict(hs=hs,octs=octs,flat=flat,dev=np.mean(devs),corr=np.corrcoef(L,R)[0,1])
T=feat('trecho.wav',0,1.23)
def err(F):
    e_oct=np.sqrt(np.mean((F['octs']-T['octs'])**2))
    e_h=np.sqrt(np.mean((F['hs']-T['hs'])**2))
    return e_oct + 0.5*e_h + 6*abs(F['flat']-T['flat']) + 0.2*abs(F['dev']-T['dev']) + 8*abs(F['corr']-T['corr']), e_oct, e_h
print("alvo: oitavas",np.round(T['octs'],1),"planicidade log %.2f desvio %.1f corr %.2f"%(T['flat'],T['dev'],T['corr']))
res=[]
grid=itertools.product([0,0.2,0.35,0.5,0.65,0.8],[2500,4500,8000,14000],[1500,3500,8000,16000],[0.3,0.5],[0.3,0.8],[80,200])
for drive,tone,cutoff,chomix,mix2,hpf in grid:
    args=[RENDER,"fit.wav","6","drive=%g"%drive,"drivetone=%g"%tone,"cutoff=%g"%cutoff,"chomix=%g"%chomix,"mix2=%g"%mix2,"hpf=%g"%hpf,"45","57","64","68","71"]
    subprocess.run(args,check=True)
    F=feat('fit.wav',2.0,3.23); e,eo,eh=err(F)
    res.append((e,drive,tone,cutoff,chomix,mix2,hpf,eo,eh,F['flat'],F['dev'],F['corr']))
res.sort()
for r in res[:8]: print("erro %.1f | drive %g tom %g corte %g chorus %g mix2 %g hpf %g | oitavas %.1f dB harm %.1f dB plan %.2f desvio %.1f corr %.2f"%r)
nd=[r for r in res if r[1]==0]; print("melhor SEM drive: erro %.1f | tom %g corte %g ... oitavas %.1f dB plan %.2f"%(nd[0][0],nd[0][2],nd[0][3],nd[0][7],nd[0][9]))
json.dump([list(map(float,r)) for r in res[:20]],open('fit2.json','w'))
