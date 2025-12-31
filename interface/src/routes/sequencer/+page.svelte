<script lang="ts">
    import SourceTargetSequencerComp from '$lib/components/SourceTargetSequencerComp.svelte';
    import type { PageData } from '../$types';
    export let data: PageData;
</script>

<div class="mx-0 my-1 flex flex-col space-y-4 sm:mx-8 sm:my-8">
    <SourceTargetSequencerComp
        label="Heliostat"
        restPath="/rest/heliostat/targets/sequence"
    />
</div>
