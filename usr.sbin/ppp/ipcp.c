/*
 *	PPP IP Control Protocol (IPCP) Module
 *
 *	    Written by Toshiharu OHNO (tony-o@iij.ad.jp)
 *
 *   Copyright (C) 1993, Internet Initiative Japan, Inc. All rights reserverd.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the Internet Initiative Japan, Inc.  The name of the
 * IIJ may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Id: ipcp.c,v 1.4.4.1 1995/08/26 12:11:41 davidg Exp $
 *
 *	TODO:
 *		o More RFC1772 backwoard compatibility
 */
#include "fsm.h"
#include "lcpproto.h"
#include "lcp.h"
#include "ipcp.h"
#include <netdb.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "slcompress.h"
#include "os.h"
#include "phase.h"
#include "vars.h"

extern void PutConfValue();
extern void Prompt();
extern struct in_addr ifnetmask;

struct ipcpstate IpcpInfo;
struct in_range DefMyAddress, DefHisAddress, DefTriggerAddress;

static void IpcpSendConfigReq __P((struct fsm *));
static void IpcpSendTerminateAck __P((struct fsm *));
static void IpcpSendTerminateReq __P((struct fsm *));
static void IpcpDecodeConfig __P((u_char *, int, int));
static void IpcpLayerStart __P((struct fsm *));
static void IpcpLayerFinish __P((struct fsm *));
static void IpcpLayerUp __P((struct fsm *));
static void IpcpLayerDown __P((struct fsm *));
static void IpcpInitRestartCounter __P((struct fsm *));

struct pppTimer IpcpReportTimer;

static int lastInOctets, lastOutOctets;

#define	REJECTED(p, x)	(p->his_reject & (1<<x))

struct fsm IpcpFsm = {
  "IPCP",
  PROTO_IPCP,
  IPCP_MAXCODE,
  OPEN_ACTIVE,
  ST_INITIAL,
  0, 0, 0,

  0,
  { 0, 0, 0, NULL, NULL, NULL },

  IpcpLayerUp,
  IpcpLayerDown,
  IpcpLayerStart,
  IpcpLayerFinish,
  IpcpInitRestartCounter,
  IpcpSendConfigReq,
  IpcpSendTerminateReq,
  IpcpSendTerminateAck,
  IpcpDecodeConfig,
};

static char *cftypes[] = {
  "???", "IPADDRS", "COMPPROTO", "IPADDR",
};

/*
 * Function called every second. Updates connection period and idle period,
 * also update LQR information.
 */
static void
IpcpReportFunc()
{
  ipConnectSecs++;
  if (lastInOctets == ipInOctets && lastOutOctets == ipOutOctets)
    ipIdleSecs++;
  lastInOctets = ipInOctets;
  lastOutOctets = ipOutOctets;
  StopTimer(&IpcpReportTimer);
  IpcpReportTimer.state = TIMER_STOPPED;
  StartTimer(&IpcpReportTimer);
}

static void
IpcpStartReport()
{
  ipIdleSecs = ipConnectSecs = 0;
  StopTimer(&IpcpReportTimer);
  IpcpReportTimer.state = TIMER_STOPPED;
  IpcpReportTimer.load = SECTICKS;
  IpcpReportTimer.func = IpcpReportFunc;
  StartTimer(&IpcpReportTimer);
}

void
ReportIpcpStatus()
{
  struct ipcpstate *icp = &IpcpInfo;
  struct fsm *fp = &IpcpFsm;

  printf("%s [%s]\n", fp->name, StateNames[fp->state]);
  printf(" his side: %s, %lx\n",
     inet_ntoa(icp->his_ipaddr), icp->his_compproto);
  printf(" my  side: %s, %lx\n",
     inet_ntoa(icp->want_ipaddr), icp->want_compproto);
  printf("connected: %d secs, idle: %d secs\n\n", ipConnectSecs, ipIdleSecs);
  printf("Defaults:\n");
  printf(" My Address:  %s/%d\n",
     inet_ntoa(DefMyAddress.ipaddr), DefMyAddress.width);
  printf(" His Address: %s/%d\n",
     inet_ntoa(DefHisAddress.ipaddr), DefHisAddress.width);
  printf(" Negotiation: %s/%d\n",
     inet_ntoa(DefTriggerAddress.ipaddr), DefTriggerAddress.width);
}

void
IpcpDefAddress()
{
  struct hostent *hp;
  char name[200];

  bzero(&DefMyAddress, sizeof(DefMyAddress));
  bzero(&DefHisAddress, sizeof(DefHisAddress));
  bzero(&DefTriggerAddress, sizeof(DefTriggerAddress));
  if (gethostname(name, sizeof(name)) == 0) {
      hp = gethostbyname(name);
      if (hp && hp->h_addrtype == AF_INET) {
	bcopy(hp->h_addr, (char *)&DefMyAddress.ipaddr.s_addr, hp->h_length);
      }
  }
}

void
IpcpInit()
{
  struct ipcpstate *icp = &IpcpInfo;

  FsmInit(&IpcpFsm);
  bzero(icp, sizeof(struct ipcpstate));
  if ((mode & MODE_DEDICATED) && !dstsystem) {
    icp->want_ipaddr.s_addr = icp->his_ipaddr.s_addr = 0;
  } else {
    icp->want_ipaddr.s_addr = DefMyAddress.ipaddr.s_addr;
    icp->his_ipaddr.s_addr = DefHisAddress.ipaddr.s_addr;
  }

  /*
   * Some implementation of PPP are:
   *  Starting a negotiaion by require sending *special* value as my address,
   *  even though standard of PPP is defined full negotiation based.
   *  (e.g. "0.0.0.0" or Not "0.0.0.0")
   */
  if ( icp->want_ipaddr.s_addr == 0 ) {
    icp->want_ipaddr.s_addr = DefTriggerAddress.ipaddr.s_addr;
  }

  if (Enabled(ConfVjcomp))
    icp->want_compproto = (PROTO_VJCOMP << 16) | ((MAX_STATES - 1) << 8);
  else
    icp->want_compproto = 0;
  icp->heis1172 = 0;
  IpcpFsm.maxconfig = 10;
}

static void
IpcpInitRestartCounter(fp)
struct fsm *fp;
{
  fp->FsmTimer.load = VarRetryTimeout * SECTICKS;
  fp->restart = 5;
}

static void
IpcpSendConfigReq(fp)
struct fsm *fp;
{
  u_char *cp;
  struct ipcpstate *icp = &IpcpInfo;

  cp = ReqBuff;
  LogPrintf(LOG_LCP, "%s: SendConfigReq\n", fp->name);
  if (!DEV_IS_SYNC || !REJECTED(icp, TY_IPADDR))
    PutConfValue(&cp, cftypes, TY_IPADDR, 6, ntohl(icp->want_ipaddr.s_addr));
  if (icp->want_compproto && !REJECTED(icp, TY_COMPPROTO)) {
    if (icp->heis1172)
      PutConfValue(&cp, cftypes, TY_COMPPROTO, 4, icp->want_compproto >> 16);
    else
      PutConfValue(&cp, cftypes, TY_COMPPROTO, 6, icp->want_compproto);
  }
  FsmOutput(fp, CODE_CONFIGREQ, fp->reqid++, ReqBuff, cp - ReqBuff);
}

static void
IpcpSendTerminateReq(fp)
struct fsm *fp;
{
  /* XXX: No code yet */
}

static void
IpcpSendTerminateAck(fp)
struct fsm *fp;
{
  LogPrintf(LOG_LCP, "  %s: SendTerminateAck\n", fp->name);
  FsmOutput(fp, CODE_TERMACK, fp->reqid++, NULL, 0);
}

static void
IpcpLayerStart(fp)
struct fsm *fp;
{
  LogPrintf(LOG_LCP, "%s: LayerStart.\n", fp->name);
}

static void
IpcpLayerFinish(fp)
struct fsm *fp;
{
  LogPrintf(LOG_LCP, "%s: LayerFinish.\n", fp->name);
  LcpClose();
  NewPhase(PHASE_TERMINATE);
}

static void
IpcpLayerDown(fp)
struct fsm *fp;
{
  LogPrintf(LOG_LCP, "%s: LayerDown.\n", fp->name);
  StopTimer(&IpcpReportTimer);
}

/*
 *  Called when IPCP has reached to OPEN state
 */
static void
IpcpLayerUp(fp)
struct fsm *fp;
{
  char tbuff[100];

#ifdef VERBOSE
  fprintf(stderr, "%s: LayerUp(%d).\r\n", fp->name, fp->state);
#endif
  Prompt(1);
  LogPrintf(LOG_LCP, "%s: LayerUp.\n", fp->name);
  sprintf(tbuff, "myaddr = %s ", inet_ntoa(IpcpInfo.want_ipaddr));
  LogPrintf(LOG_LCP, " %s hisaddr = %s\n", tbuff, inet_ntoa(IpcpInfo.his_ipaddr));
  OsSetIpaddress(IpcpInfo.want_ipaddr, IpcpInfo.his_ipaddr, ifnetmask);
  OsLinkup();
  IpcpStartReport();
  StartIdleTimer();
}

void
IpcpUp()
{
  FsmUp(&IpcpFsm);
  LogPrintf(LOG_LCP, "IPCP Up event!!\n");
}

void
IpcpOpen()
{
  FsmOpen(&IpcpFsm);
}

static int
AcceptableAddr(prange, ipaddr)
struct in_range *prange;
struct in_addr ipaddr;
{
#ifdef DEBUG
  logprintf("requested = %x ", htonl(ipaddr.s_addr));
  logprintf("range = %x", htonl(prange->ipaddr.s_addr));
  logprintf("/%x\n", htonl(prange->mask.s_addr));
  logprintf("%x, %x\n", htonl(prange->ipaddr.s_addr & prange->mask.s_addr),
    htonl(ipaddr.s_addr & prange->mask.s_addr));
#endif
  return((prange->ipaddr.s_addr & prange->mask.s_addr) ==
	(ipaddr.s_addr & prange->mask.s_addr));
}

static void
IpcpDecodeConfig(cp, plen, mode)
u_char *cp;
int plen;
int mode;
{
  int type, length;
  u_long *lp, compproto;
  struct compreq *pcomp;
  struct in_addr ipaddr, dstipaddr;
  char tbuff[100];

  ackp = AckBuff;
  nakp = NakBuff;
  rejp = RejBuff;

  while (plen >= sizeof(struct fsmconfig)) {
    if (plen < 0)
      break;
    type = *cp;
    length = cp[1];
    if (type <= TY_IPADDR)
      sprintf(tbuff, " %s[%d] ", cftypes[type], length);
    else
      sprintf(tbuff, " ");

    switch (type) {
    case TY_IPADDR:		/* RFC1332 */
      lp = (u_long *)(cp + 2);
      ipaddr.s_addr = *lp;
      LogPrintf(LOG_LCP, "%s %s\n", tbuff, inet_ntoa(ipaddr));

      switch (mode) {
      case MODE_REQ:
	if (!AcceptableAddr(&DefHisAddress, ipaddr)) {
          /*
           * If destination address is not acceptable, insist to use
           * what we want to use.
           */
	  bcopy(cp, nakp, 2);
          bcopy(&IpcpInfo.his_ipaddr.s_addr, nakp+2, length);
          nakp += length;
          break;

	}
	IpcpInfo.his_ipaddr = ipaddr;
	bcopy(cp, ackp, length);
	ackp += length;
	break;
      case MODE_NAK:
	if (AcceptableAddr(&DefMyAddress, ipaddr)) {
          /*
           * Use address suggested by peer.
           */
	  sprintf(tbuff+50, "%s changing address: %s ", tbuff, inet_ntoa(IpcpInfo.want_ipaddr));
	  LogPrintf(LOG_LCP, "%s --> %s\n", tbuff+50, inet_ntoa(ipaddr));
	  IpcpInfo.want_ipaddr = ipaddr;
	}
	break;
      case MODE_REJ:
	IpcpInfo.his_reject |= (1 << type);
	break;
      }
      break;
    case TY_COMPPROTO:
      lp = (u_long *)(cp + 2);
      compproto = htonl(*lp);
      LogPrintf(LOG_LCP, "%s %08x\n", tbuff, compproto);

      switch (mode) {
      case MODE_REQ:
	if (!Acceptable(ConfVjcomp)) {
	  bcopy(cp, rejp, length);
	  rejp += length;
	} else {
	  pcomp = (struct compreq *)(cp + 2);
	  switch (length) {
	  case 4:	/* RFC1172 */
	    if (ntohs(pcomp->proto) == PROTO_VJCOMP) {
	      logprintf("** Peer is speaking RFC1172 compression protocol **\n");
	      IpcpInfo.heis1172 = 1;
	      IpcpInfo.his_compproto = compproto;
	      bcopy(cp, ackp, length);
	      ackp += length;
	    } else {
	      bcopy(cp, nakp, 2);
	      pcomp->proto = htons(PROTO_VJCOMP);
	      bcopy(&pcomp, nakp + 2, 2);
	      nakp += length;
	    }
	    break;
	  case 6: 	/* RFC1332 */
	    if (ntohs(pcomp->proto) == PROTO_VJCOMP
	        && pcomp->slots < MAX_STATES && pcomp->slots > 2) {
	      IpcpInfo.his_compproto = compproto;
	      IpcpInfo.heis1172 = 0;
	      bcopy(cp, ackp, length);
	      ackp += length;
	    } else {
	      bcopy(cp, nakp, 2);
	      pcomp->proto = htons(PROTO_VJCOMP);
	      pcomp->slots = MAX_STATES - 1;
	      pcomp->compcid = 0;
	      bcopy(&pcomp, nakp + 2, sizeof(pcomp));
	      nakp += length;
	    }
	    break;
	  default:
	    bcopy(cp, rejp, length);
	    rejp += length;
	    break;
	  }
	}
	break;
      case MODE_NAK:
	LogPrintf(LOG_LCP, "%s changing compproto: %08x --> %08x\n",
	  tbuff, IpcpInfo.want_compproto, compproto);
	IpcpInfo.want_compproto = compproto;
	break;
      case MODE_REJ:
	IpcpInfo.his_reject |= (1 << type);
	break;
      }
      break;
    case TY_IPADDRS:	/* RFC1172 */
      lp = (u_long *)(cp + 2);
      ipaddr.s_addr = *lp;
      lp = (u_long *)(cp + 6);
      dstipaddr.s_addr = *lp;
      LogPrintf(LOG_LCP, "%s %s, ", tbuff, inet_ntoa(ipaddr));
      LogPrintf(LOG_LCP, "%s\n", inet_ntoa(dstipaddr));

      switch (mode) {
      case MODE_REQ:
	IpcpInfo.his_ipaddr = ipaddr;
	IpcpInfo.want_ipaddr = dstipaddr;
	bcopy(cp, ackp, length);
	ackp += length;
	break;
      case MODE_NAK:
	LogPrintf(LOG_LCP, "%s changing address: %s ",
	  tbuff, inet_ntoa(IpcpInfo.want_ipaddr));
	LogPrintf(LOG_LCP, "--> %s\n", inet_ntoa(ipaddr));
	IpcpInfo.want_ipaddr = ipaddr;
	IpcpInfo.his_ipaddr = dstipaddr;
	break;
      case MODE_REJ:
	IpcpInfo.his_reject |= (1 << type);
	break;
      }
      break;
    default:
      IpcpInfo.my_reject |= (1 << type);
      bcopy(cp, rejp, length);
      rejp += length;
      break;
    }
    plen -= length;
    cp += length;
  }
}

void
IpcpInput(struct mbuf *bp)
{
  FsmInput(&IpcpFsm, bp);
}
