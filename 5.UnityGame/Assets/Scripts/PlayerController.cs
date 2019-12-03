using System.Collections;
using System.Collections.Generic;
using UnityEngine.Networking;
using UnityEngine;

public class PlayerController : NetworkBehaviour
{
    private Animator animator;
    private Camera mainCamera;
    private TextMesh nameLabel;

    const float RUNNING_SPEED = 10.0f;
    const float ROTATION_SPEED = 180.0f;

    CustomNetworkManager networkManager;

    // Name sync /////////////////////////////////////

        [SyncVar(hook = "SyncNameChanged")]
    public string playerName = "Player";

    [Command]
    void CmdChangeName(string name) { playerName = name; }

    void SyncNameChanged(string name) { nameLabel.text = name; } // the hook of the playerName var


    // Index player Sync //////////////////////////////

    [Command]

    void CmdChangePlayerPrefab(int prefabIndex)
    {
        networkManager.ChangePlayerPrefab(this, prefabIndex);
    }

   
    // OnGUI /////////////////////////////////////////

    private void OnGUI()
    {
        if (isLocalPlayer)
        {
            GUILayout.BeginArea(new Rect(Screen.width - 260, 10, 250, Screen.height - 20));

            string prevPlayerName = playerName;
            playerName = GUILayout.TextField(playerName);
            if (playerName != prevPlayerName)
            {
                CmdChangeName(playerName);
            }

            GUILayout.EndArea();

            short newIndex = (short)GUILayout.SelectionGrid(
                networkManager.playerPrefabIndex, networkManager.playerNames, 3);
            if (newIndex != networkManager.playerPrefabIndex)
            {
                networkManager.playerPrefabIndex = newIndex;
                CmdChangePlayerPrefab(newIndex);
                
            }
                
        }
    }


    // Animation sync ////////////////////////////////
    
        [SyncVar(hook ="OnSetAnimation")]
    string animationName;

    void setAnimation(string animName)
    {
        OnSetAnimation(animName);
        CmdSetAnimation(animName);
    }

    [Command]
    void CmdSetAnimation(string name)
    {

        animationName = name;
    }

    void OnSetAnimation(string animName)
    {
        if (animationName == animName) return;
        animationName = animName;

        animator.SetBool("Idling", false);
        animator.SetBool("Running", false);
        animator.SetBool("Running backwards", false);
        animator.ResetTrigger("Jumping");
        animator.ResetTrigger("Kicking");

        if (animationName == "Idling") animator.SetBool("Idling", true);
        else if (animationName == "Running") animator.SetBool("Running", true);
        else if (animationName == "Running backwards") animator.SetBool("Running backwards", true);
        else if (animationName == "Jumping") animator.SetTrigger("Jumping");
        else if (animationName == "Kicking") animator.SetTrigger("Kicking");
    }


    // Lifecycle methods ////////////////////////////

    // Use this for initialization
    void Start ()
    {
        animator = GetComponent<Animator>();
        mainCamera = Camera.main;
        nameLabel = transform.Find("Label").gameObject.GetComponent<TextMesh>();

        NetworkManager mng = NetworkManager.singleton;
        networkManager = mng.GetComponent<CustomNetworkManager>();
    }

    // Update is called once per frame
    void Update()
    {
        if (isLocalPlayer)
        {
            Vector3 translation = new Vector3();
            float angle = 0.0f;

            float horizontalAxis = Input.GetAxis("Horizontal");
            float verticalAxis = Input.GetAxis("Vertical");

            if (verticalAxis > 0.0)
            {
                setAnimation("Running");
                translation += new Vector3(0.0f, 0.0f, verticalAxis * RUNNING_SPEED * Time.deltaTime);
                transform.Translate(translation);
            }
            else if (verticalAxis < 0.0)
            {
                setAnimation("Running backwards");
                translation += new Vector3(0.0f, 0.0f, verticalAxis * RUNNING_SPEED * Time.deltaTime * 0.5f);
                transform.Translate(translation);
            }
            else
            {
                setAnimation("Idling");
            }

            if (horizontalAxis > 0.0f)
            {
                angle = horizontalAxis * Time.deltaTime * ROTATION_SPEED;
                transform.Rotate(new Vector3(0.0f, 1.0f, 0.0f), angle);
            }
            else if (horizontalAxis < 0.0f)
            {
                angle = horizontalAxis * Time.deltaTime * ROTATION_SPEED;
                transform.Rotate(new Vector3(0.0f, 1.0f, 0.0f), angle);
            }

            if (Input.GetButtonDown("Jump"))
            {
                setAnimation("Jumping");
            }

            if (Input.GetButtonDown("Fire1"))
            {
                setAnimation("Kicking");
            }

            if (mainCamera)
            {
                mainCamera.transform.SetPositionAndRotation(transform.position + new Vector3(0.0f, 4.0f, -3.0f), Quaternion.identity);
                mainCamera.transform.LookAt(transform.position + new Vector3(0.0f, 2.0f, 0.0f), Vector3.up);
            }
        }
        if (nameLabel)
        {
            nameLabel.transform.rotation = Quaternion.identity;
        }
    }

    private void OnDestroy()
    {
    }
}
