void onTrigger()
{

	TrackObject @t_obj = getTrackObject("anim_sheep2.b3d");
     //t_obj.setEnable(false);
     Mesh @sheepMesh = t_obj.getMesh();
     displayMessage("moo");
     sheepMesh.setLoop(1,3); //rapid-nod sheep
     
}
