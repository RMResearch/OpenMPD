using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class MiniScreenVideoImageSelector : MonoBehaviour
{
    public MediaOption media = MediaOption.image;
    public GameObject image;
    public GameObject video;
    Quaternion imageRot;
    Quaternion videoRot;

    MediaOption prevMedia;
    //bool updateMedia = false;

    private void OnEnable()
    {
        imageRot = image.transform.rotation;
        videoRot = video.transform.rotation;
    }

    // Start is called before the first frame update
    void Start()
    {
        UpdateMediaSelection();
    }

    // Update is called once per frame
    void Update()
    {
        if(CheckMediaChanges())//updateMedia
        {
            UpdateMediaSelection();            
            //updateMedia = false;
        }
    }

    void UpdateMediaSelection()
    {
        if (media == MediaOption.image)
        {
            //GetComponentInChildren<UpdateImage>().gameObject.SetActive(true);
            //GetComponentInChildren<VideoProxy>().gameObject.SetActive(false);
            image.SetActive(true);
            video.SetActive(false);
            image.transform.rotation = imageRot;
        }
        else if (media == MediaOption.video)
        {
            //GetComponentInChildren<UpdateImage>().gameObject.SetActive(false);
            //GetComponentInChildren<VideoProxy>().gameObject.SetActive(true);
            image.SetActive(false);
            video.SetActive(true);
            video.transform.rotation = videoRot;
        }

        prevMedia = media;
    }

    bool CheckMediaChanges()
    {
        if (media == prevMedia)
            return false;
        else
            return true;
    }
}
