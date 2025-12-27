<script lang="ts">
	import { onMount, onDestroy } from 'svelte';
    import Collapsible from '$lib/components/Collapsible.svelte';
	import type { ServoControllerState } from '$lib/types/models'
	import Slider from '$lib/components/Slider.svelte';
	import Checkbox from '$lib/components/Checkbox.svelte';
	import GridForm from '$lib/components/GridForm.svelte';
	import Spinner from '$lib/components/Spinner.svelte';
	import SettingsCard from './SettingsCard.svelte';
	import { getJsonRest, postJsonRest } from '$lib/stores/rest';
	import DisableButton from './DisableButton.svelte';
	import StopButton from './StopButton.svelte';

    export let label: string;
    export let restPath : string;

    let controllerState : ServoControllerState = {
        position: 0,
        target: 0,
        tolerance: 0.1,
        offset: 0,
        enabled: false,
        invert: false,
        plot: false,
        encoderError: false,
        P: 10,
        I: 0,
        D: 0,
        S: 0,
        curGain: 0,
        derivative: 0,
        integral: 0,
        limits: {
            enabled: false,
            begin: 0,
            end: 360
        },
    };

    let intervalID: any;
    onMount(() => {
        intervalID = setInterval(()=>{
            getPosition();
        }, 987);
    });
    onDestroy(()=> {
        clearInterval(intervalID);
    });

    async function getPosition() {
        getJsonRest(restPath + "/position", controllerState.position).then((data) => controllerState.position = data);
    }

    async function getControllerState() {
        return getJsonRest(restPath, controllerState).then((data) => controllerState = data);
    }

    function postControllerState() {
        postJsonRest(restPath, controllerState).then((data) => controllerState = data);
    }
</script>

<SettingsCard>
	<span slot="title">{label} Controller</span>
    {#await getControllerState()}
    <Spinner></Spinner>
    {:then nothing}
        <div>
            <GridForm>
                <Slider 
                    label="Position" 
                    bind:value={controllerState.position}
                    min={controllerState.limits.enabled ? (controllerState.limits.begin < controllerState.limits.end ? controllerState.limits.begin : controllerState.limits.end) : 0} 
                    max={controllerState.limits.enabled ? (controllerState.limits.begin > controllerState.limits.end ? controllerState.limits.begin : controllerState.limits.end) : 360}
                    step={0.01}
                    disabled
                ></Slider>
                <Slider 
                    label="Target" 
                    bind:value={controllerState.target}
                    min={controllerState.limits.enabled ? (controllerState.limits.begin < controllerState.limits.end ? controllerState.limits.begin : controllerState.limits.end) : 0} 
                    max={controllerState.limits.enabled ? (controllerState.limits.begin > controllerState.limits.end ? controllerState.limits.begin : controllerState.limits.end) : 360}
                    step={0.01}
                    onChange={() => postJsonRest(restPath, {target: controllerState.target})}
                ></Slider>
            </GridForm>
            <Collapsible>
                <span slot="title">Settings</span>
                <span class="text-lg">Control</span>
                <GridForm>
                    <Checkbox 
                        label="Enable" 
                        bind:value={controllerState.enabled}
                        onChange={postControllerState}
                    ></Checkbox>
                    <Checkbox 
                        label="Invert" 
                        bind:value={controllerState.invert}
                        onChange={postControllerState}
                    ></Checkbox>
                    <Slider 
                        label="Offset" 
                        bind:value={controllerState.offset}
                        min={0} 
                        max={360} 
                        step={0.01}
                        onChange={postControllerState}
                    ></Slider>
                    <Slider 
                        label="Tolerance" 
                        bind:value={controllerState.tolerance}
                        min={0.04} 
                        max={0.4} 
                        step={0.01}
                        onChange={postControllerState}
                    ></Slider>
                    <Slider 
                        label="P Gain" 
                        bind:value={controllerState.P}
                        min={0} 
                        max={20} 
                        step={0.1}
                        onChange={postControllerState}
                    ></Slider>
                    <Slider 
                        label="I Gain" 
                        bind:value={controllerState.I}
                        min={0} 
                        max={1} 
                        step={0.01}
                        onChange={postControllerState}
                    ></Slider>
                    <Slider 
                        label="D Gain" 
                        bind:value={controllerState.D}
                        min={0} 
                        max={1} 
                        step={0.01}
                        onChange={postControllerState}
                    ></Slider>
                    <Slider 
                        label="S Gain" 
                        bind:value={controllerState.S}
                        min={0} 
                        max={1} 
                        step={0.01}
                        onChange={postControllerState}
                    ></Slider>
                    <Checkbox 
                        label="Plot" 
                        bind:value={controllerState.plot}
                        onChange={postControllerState}
                    ></Checkbox>
                </GridForm>
                <span class="text-lg">Limits</span>
                <GridForm>
                    <Checkbox 
                        label="Enable" 
                        bind:value={controllerState.limits.enabled}
                        onChange={postControllerState}
                    ></Checkbox>
                    {#if controllerState.limits.enabled}
                    <Slider 
                        label="Begin" 
                        bind:value={controllerState.limits.begin}
                        min={0} 
                        max={360} 
                        step={0.01}
                        onChange={postControllerState}
                    ></Slider>
                    <Slider 
                        label="End" 
                        bind:value={controllerState.limits.end}
                        min={0} 
                        max={360} 
                        step={0.01}
                        onChange={postControllerState}
                    ></Slider>
                    {/if}
                </GridForm>
            </Collapsible>
        </div>
    {/await}
    <div class="flex flex-row flex-wrap justify-between gap-x-2">
        <div class="flex-grow"></div>
        <div>
            <div>
                <DisableButton onClick={() => postJsonRest(restPath, {enabled: false})}></DisableButton>
                <StopButton onClick={() => postJsonRest(restPath, {enabled: false})}></StopButton>
            </div>
        </div>
    </div>
</SettingsCard>