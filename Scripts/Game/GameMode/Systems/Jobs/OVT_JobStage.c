class OVT_JobStage : ScriptAndConfig
{	
	//return false to go to next stage/end job
	bool OnStart(OVT_Job job)
	{
		return true;
	}
	
	//return false to go to next stage/end job
	bool OnTick(OVT_Job job)
	{
		return true;
	}
	
	void OnEnd(OVT_Job job)
	{
	
	}
}