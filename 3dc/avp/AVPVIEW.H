#ifndef _included_avpview_h_ /* Is this your first time? */
#define _included_avpview_h_ 1

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/* KJL 10:49:41 04/21/97 - avpview.h */
extern void AvpShowViews(void);
extern void InitCameraValues(void);
extern void LightSourcesInRangeOfObject(DISPLAYBLOCK *dptr);
void MakeViewingWindowLarger(void);
void MakeViewingWindowSmaller(void);

extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
