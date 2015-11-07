int temp[] = {27,100,250,260,270,280,290,300,310,320,330,340,350,360,370,380,390,400,410,420,430,440,450};
int adIn[] = {460,550,656,663,668,673,678,682,688,693,698,703,708,712,717,722,726,730,735,739,745,751,758};

int multiMap(int val)
{
  int size = sizeof(adIn)/sizeof(int);
  
  if (val <= adIn[0]) return temp[0];
  if (val >= adIn[size-1]) return temp[size-1];

  int pos = 1;  
  while(val > adIn[pos]) pos++;
  if (val == adIn[pos]) return temp[pos];

  return (val - adIn[pos-1]) * (temp[pos] - temp[pos-1]) / (adIn[pos] - adIn[pos-1]) + temp[pos-1];
}


